#include <memory>
#include <thread>
#include <mutex>
#include <functional>

#include <ros/ros.h>
#include <nodelet/nodelet.h>
#include <image_transport/image_transport.h>
#include <image_transport/subscriber_filter.h>
#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <sensor_msgs/image_encodings.h>
#include <sensor_msgs/point_cloud2_iterator.h>
#include <sensor_msgs/PointCloud2.h>
#include <image_geometry/pinhole_camera_model.h>
#include <cv_bridge/cv_bridge.h>
#include <geometry_msgs/TransformStamped.h>
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

namespace structure_core
{
  using namespace message_filters::sync_policies;
  namespace enc = sensor_msgs::image_encodings;

  typedef sensor_msgs::PointCloud2 PointCloud;

  tf2::Transform msg_to_tf(geometry_msgs::TransformStamped &msg) {
    return tf2::Transform(
            tf2::Quaternion(
              msg.transform.rotation.x,
              msg.transform.rotation.y,
              msg.transform.rotation.z,
              msg.transform.rotation.w),
            tf2::Vector3(
              msg.transform.translation.x,
              msg.transform.translation.y,
              msg.transform.translation.z));
  } 

class PointCloudXyzrgbNodelet : public nodelet::Nodelet
{
public:

  ros::NodeHandlePtr rgb_nh_;
  ros::NodeHandlePtr depth_nh_;
  ros::NodeHandlePtr rgbd_nh;
  std::shared_ptr<image_transport::ImageTransport> rgb_it_, depth_it_;
  
  image_transport::SubscriberFilter sub_depth_, sub_rgb_;
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_;
  geometry_msgs::TransformStamped transform;
  bool find_transfrom;
  message_filters::Subscriber<sensor_msgs::CameraInfo> sub_depth_info_;
  message_filters::Subscriber<sensor_msgs::CameraInfo> sub_vis_info_;
  typedef ApproximateTime<sensor_msgs::Image, sensor_msgs::Image, sensor_msgs::CameraInfo, sensor_msgs::CameraInfo> SyncPolicy;
  typedef message_filters::Synchronizer<SyncPolicy> Synchronizer;
  std::shared_ptr<Synchronizer> sync_;

  std::mutex connect_mutex_;
  typedef sensor_msgs::PointCloud2 PointCloud;
  ros::Publisher pub_point_cloud_;

  image_geometry::PinholeCameraModel depth_model_;
  image_geometry::PinholeCameraModel vis_model_;

  //params
  int queue_size;
  bool remove_nans;
  bool rviz_frame;
  std::string name;

  virtual void onInit();

  void connectCb();

  void imageCb(const sensor_msgs::ImageConstPtr& depth_msg,
               const sensor_msgs::ImageConstPtr& rgb_msg,
               const sensor_msgs::CameraInfoConstPtr& info_depth_msg,
               const sensor_msgs::CameraInfoConstPtr& info_vis_msg);

  template<typename T>
  void convert(const sensor_msgs::ImageConstPtr& depth_msg,
               const sensor_msgs::ImageConstPtr& rgb_msg,
               const PointCloud::Ptr& cloud_msg,
               int red_offset, int green_offset, int blue_offset, int color_step);


};

void PointCloudXyzrgbNodelet::onInit()
{
  NODELET_DEBUG("PointCloudXyzrgbNodelet - %s", __FUNCTION__);

  ros::NodeHandle& nh  = getNodeHandle();
  ros::NodeHandle& pnh = getPrivateNodeHandle();

  // Read parameters
  
  pnh.param<int>("queue_size", queue_size, 10);
  pnh.param<bool>("remove_nans", remove_nans, false);
  pnh.param<bool>("rviz_frame", rviz_frame, false);
  pnh.param<std::string>("name", name, "sc");

  find_transfrom = false;

  rgb_nh_.reset( new ros::NodeHandle(nh, name + "/rgb") );
  depth_nh_.reset( new ros::NodeHandle(nh, name + "/depth") );
  rgbd_nh.reset( new ros::NodeHandle(nh, name + "/rgbd") );
  rgb_it_  .reset( new image_transport::ImageTransport(*rgb_nh_) );
  depth_it_.reset( new image_transport::ImageTransport(*depth_nh_) );
  tf_buffer_.reset( new tf2_ros::Buffer );
  tf_.reset( new tf2_ros::TransformListener(*tf_buffer_) );

  sync_.reset( new Synchronizer(SyncPolicy(queue_size), sub_depth_, sub_rgb_, sub_depth_info_,  sub_vis_info_) );
  sync_->registerCallback(boost::bind(&PointCloudXyzrgbNodelet::imageCb, this, _1, _2, _3, _4));

  ros::SubscriberStatusCallback connect_cb = boost::bind(&PointCloudXyzrgbNodelet::connectCb, this);
  std::lock_guard<std::mutex> lock(connect_mutex_);

  pub_point_cloud_ = rgbd_nh->advertise<PointCloud>("points", 1, connect_cb, connect_cb); 
}

void PointCloudXyzrgbNodelet::connectCb()
{
  std::lock_guard<std::mutex> lock(connect_mutex_);
  if (pub_point_cloud_.getNumSubscribers() == 0)
  {
    sub_depth_.unsubscribe();
    sub_rgb_.unsubscribe();
    sub_depth_info_.unsubscribe();
    sub_vis_info_.unsubscribe();
  }
  else if (!sub_depth_.getSubscriber())
  {
    ros::NodeHandle& private_nh = getPrivateNodeHandle();

    std::string depth_image_transport_param = "depth_image_transport";
    image_transport::TransportHints depth_hints("raw",ros::TransportHints(), private_nh, depth_image_transport_param);
    sub_depth_.subscribe(*depth_it_, "image",       1, depth_hints);

    image_transport::TransportHints hints("raw", ros::TransportHints(), private_nh);
    sub_rgb_  .subscribe(*rgb_it_,   "image", 1, hints);
    sub_depth_info_ .subscribe(*depth_nh_,   "camera_info",      1);
    sub_vis_info_ .subscribe(*rgb_nh_,   "camera_info",      1);
  }
}

void PointCloudXyzrgbNodelet::imageCb(const sensor_msgs::ImageConstPtr& depth_msg,
                                      const sensor_msgs::ImageConstPtr& rgb_msg,
                                      const sensor_msgs::CameraInfoConstPtr& info_depth_msg,
                                      const sensor_msgs::CameraInfoConstPtr& info_vis_msg)
{
  NODELET_DEBUG("PointCloudXyzrgbNodelet - %s", __FUNCTION__);
  try
  {
    if(!find_transfrom)
    {
      transform = tf_buffer_->lookupTransform (
                          info_depth_msg->header.frame_id, info_vis_msg->header.frame_id, 
                          info_depth_msg->header.stamp);

      tf2::Transform transform_tf = msg_to_tf(transform);
      find_transfrom = true;
    }
  }
  catch (tf2::TransformException& ex)
  {
    NODELET_WARN_THROTTLE(2, "TF2 exception:\n%s", ex.what());
    return;
  }

  // Update camera model
  depth_model_.fromCameraInfo(info_depth_msg);
  vis_model_.fromCameraInfo(info_vis_msg);

  // Supported color encodings: RGB8, MONO8
  int red_offset, green_offset, blue_offset, color_step;
  if (rgb_msg->encoding == enc::RGB8)
  {
    red_offset   = 0;
    green_offset = 1;
    blue_offset  = 2;
    color_step   = 3;
  }
  else if (rgb_msg->encoding == enc::MONO8)
  {
    red_offset   = 0;
    green_offset = 0;
    blue_offset  = 0;
    color_step   = 1;
  }

  // Allocate new point cloud message
  PointCloud::Ptr cloud_msg (new PointCloud);

  if (depth_msg->encoding == enc::TYPE_16UC1)
  {
    convert<uint16_t>(depth_msg, rgb_msg, cloud_msg, red_offset, green_offset, blue_offset, color_step);
  }
  else if (depth_msg->encoding == enc::TYPE_32FC1)
  {
    convert<float>(depth_msg, rgb_msg, cloud_msg, red_offset, green_offset, blue_offset, color_step);
  }
  else
  {
    NODELET_ERROR_THROTTLE(5, "Depth image has unsupported encoding [%s]", depth_msg->encoding.c_str());
    return;
  }

  pub_point_cloud_.publish (cloud_msg);
}

template<typename T>
void PointCloudXyzrgbNodelet::convert(const sensor_msgs::ImageConstPtr& depth_msg,
                                      const sensor_msgs::ImageConstPtr& rgb_msg,
                                      const PointCloud::Ptr& cloud_msg,
                                      int red_offset, int green_offset, int blue_offset, int color_step)
{

  float vis_cx = vis_model_.cx();
  float vis_cy = vis_model_.cy();
  float vis_fx = vis_model_.fx();
  float vis_fy = vis_model_.fy();

  float depth_cx = depth_model_.cx();
  float depth_cy = depth_model_.cy();
  float depth_fx = depth_model_.fx();
  float depth_fy = depth_model_.fy();
  
  const T* depth_row = reinterpret_cast<const T*>(&depth_msg->data[0]);
  int row_step = depth_msg->step / sizeof(T);
  const uint8_t* rgb = &rgb_msg->data[0];
  int rgb_skip = rgb_msg->step - rgb_msg->width * color_step;

  int pcl_width = 0;

  if(remove_nans)
  {
    for (int v = 0; v < int(depth_msg->height); ++v, depth_row += row_step)
    {
      for (int u = 0; u < int(depth_msg->width); ++u)
      {
        T depth = depth_row[u];

        if (!std::isnan(depth))
          pcl_width++;
      }
    }
  }
  else
  {
    pcl_width = depth_msg->height * depth_msg->width;
  }

  depth_row = reinterpret_cast<const T*>(&depth_msg->data[0]);
  
  float bad_point = std::numeric_limits<float>::quiet_NaN ();

  // Use depth image time stamp
  cloud_msg->header.stamp = depth_msg->header.stamp;
  cloud_msg->header.frame_id = depth_msg->header.frame_id;
  cloud_msg->height = 1;
  cloud_msg->width  = pcl_width;
  cloud_msg->is_dense = false;
  cloud_msg->is_bigendian = false;

  NODELET_DEBUG("pcl_width - %d", pcl_width);

  sensor_msgs::PointCloud2Modifier pcd_modifier(*cloud_msg);
  pcd_modifier.setPointCloud2FieldsByString(2, "xyz", "rgb");
  pcd_modifier.resize(pcl_width);

  sensor_msgs::PointCloud2Iterator<float> iter_x(*cloud_msg, "x");
  sensor_msgs::PointCloud2Iterator<float> iter_y(*cloud_msg, "y");
  sensor_msgs::PointCloud2Iterator<float> iter_z(*cloud_msg, "z");
  sensor_msgs::PointCloud2Iterator<uint8_t> iter_r(*cloud_msg, "r");
  sensor_msgs::PointCloud2Iterator<uint8_t> iter_g(*cloud_msg, "g");
  sensor_msgs::PointCloud2Iterator<uint8_t> iter_b(*cloud_msg, "b");
  sensor_msgs::PointCloud2Iterator<uint8_t> iter_a(*cloud_msg, "a");

  for (int v = 0; v < int(depth_msg->height); ++v, depth_row += row_step, rgb += rgb_skip)
  {
    for (int u = 0; u < int(depth_msg->width); ++u )
    {
      T depth = depth_row[u];

      if(std::isnan(depth))
      {
        if(!remove_nans)
        {    
          *iter_x = bad_point;
          *iter_y = bad_point;
          *iter_z = bad_point;
          *iter_a = bad_point;
          *iter_r = bad_point;
          *iter_g = bad_point;
          *iter_b = bad_point;

          ++iter_x, ++iter_y, ++iter_z, ++iter_a, ++iter_r, ++iter_g, ++iter_b;
        }
      }
      else
      {
        auto depth_in_meters = depth_row[u];

        float x = depth_in_meters * (u - depth_cx) / depth_fx;
        float y = depth_in_meters * (v - depth_cy) / depth_fy;
        float z = depth_in_meters;

        geometry_msgs::PointStamped initial_pt;
        initial_pt.point.x = x;
        initial_pt.point.y = y;
        initial_pt.point.z = z;

        geometry_msgs::PointStamped  transformed_pt; 

        tf2::doTransform(initial_pt, transformed_pt, transform);

        float float_visible_col = (transformed_pt.point.x * vis_fx / transformed_pt.point.z) + vis_cx;
        float float_visible_row = (transformed_pt.point.y * vis_fy / transformed_pt.point.z) + vis_cy;

        int visible_col = int(float_visible_col);
        int visible_row = int(float_visible_row);

        std::size_t pixel_color_offset = (visible_row*rgb_msg->width * color_step) + visible_col * color_step;

        if(rviz_frame)
        {
          *iter_x = z;
          *iter_y = -x;
          *iter_z = -y;

        }
        else
        {
          *iter_x = x;
          *iter_y = y;
          *iter_z = z; 
        }
        
        // Fill in color
        *iter_a = 255;
        *iter_r = rgb[pixel_color_offset+red_offset];
        *iter_g = rgb[pixel_color_offset+green_offset];
        *iter_b = rgb[pixel_color_offset+blue_offset];

        ++iter_x, ++iter_y, ++iter_z, ++iter_a, ++iter_r, ++iter_g, ++iter_b;
      }
    }
  } 
}
};

#include <pluginlib/class_list_macros.h>

PLUGINLIB_EXPORT_CLASS(structure_core::PointCloudXyzrgbNodelet, nodelet::Nodelet)
