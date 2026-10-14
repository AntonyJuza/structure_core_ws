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

namespace structure_core
{
  using namespace message_filters::sync_policies;
  namespace enc = sensor_msgs::image_encodings;

  typedef sensor_msgs::PointCloud2 PointCloud;

class PointCloudXyzNodelet : public nodelet::Nodelet
{
public:

  ros::NodeHandlePtr depth_nh_;
  std::shared_ptr<image_transport::ImageTransport>  depth_it_;
  
  image_transport::SubscriberFilter sub_depth_;
  message_filters::Subscriber<sensor_msgs::CameraInfo> sub_depth_info_;
  typedef ApproximateTime<sensor_msgs::Image, sensor_msgs::CameraInfo> SyncPolicy;
  typedef message_filters::Synchronizer<SyncPolicy> Synchronizer;
  std::shared_ptr<Synchronizer> sync_;

  std::mutex connect_mutex_;
  typedef sensor_msgs::PointCloud2 PointCloud;
  ros::Publisher pub_point_cloud_;

  image_geometry::PinholeCameraModel depth_model_;

  int queue_size;
  bool remove_nans;
  bool rviz_frame;
  std::string name;

  virtual void onInit();

  void connectCb();

  void imageCb(const sensor_msgs::ImageConstPtr& depth_msg,
               const sensor_msgs::CameraInfoConstPtr& info_depth_msg);

  template<typename T>
  void convert(const sensor_msgs::ImageConstPtr& depth_msg,
               const PointCloud::Ptr& cloud_msg);


};

void PointCloudXyzNodelet::onInit()
{
  NODELET_DEBUG("PointCloudXyzNodelet - %s", __FUNCTION__);

  ros::NodeHandle& nh  = getNodeHandle();
  ros::NodeHandle& pnh = getPrivateNodeHandle();

  // Read parameters

  pnh.param<int>("queue_size", queue_size, 10);
  pnh.param<bool>("remove_nans", remove_nans, false);
  pnh.param<bool>("rviz_frame", rviz_frame, false);
  pnh.param<std::string>("name", name, "sc");

  depth_nh_.reset( new ros::NodeHandle(nh, name + "/depth") );
  depth_it_.reset( new image_transport::ImageTransport(*depth_nh_) );

  sync_.reset( new Synchronizer(SyncPolicy(queue_size), sub_depth_, sub_depth_info_) );
  sync_->registerCallback(boost::bind(&PointCloudXyzNodelet::imageCb, this, _1, _2));

  ros::SubscriberStatusCallback connect_cb = boost::bind(&PointCloudXyzNodelet::connectCb, this);
  std::lock_guard<std::mutex> lock(connect_mutex_);

  pub_point_cloud_ = depth_nh_->advertise<PointCloud>("points", 1, connect_cb, connect_cb); 
}

void PointCloudXyzNodelet::connectCb()
{
  std::lock_guard<std::mutex> lock(connect_mutex_);
  if (pub_point_cloud_.getNumSubscribers() == 0)
  {
    sub_depth_.unsubscribe();
    sub_depth_info_.unsubscribe();
  }
  else if (!sub_depth_.getSubscriber())
  {
    ros::NodeHandle& private_nh = getPrivateNodeHandle();

    std::string depth_image_transport_param = "depth_image_transport";
    image_transport::TransportHints depth_hints("raw",ros::TransportHints(), private_nh, depth_image_transport_param);
    sub_depth_.subscribe(*depth_it_, "image",       1, depth_hints);
    sub_depth_info_ .subscribe(*depth_nh_,   "camera_info",      1);
  }
}

void PointCloudXyzNodelet::imageCb(const sensor_msgs::ImageConstPtr& depth_msg,
                                      const sensor_msgs::CameraInfoConstPtr& info_depth_msg)
{
  NODELET_DEBUG("PointCloudXyzNodelet - %s", __FUNCTION__);

  // Update camera model
  depth_model_.fromCameraInfo(info_depth_msg);

  // Allocate new point cloud message
  PointCloud::Ptr cloud_msg (new PointCloud);

  if (depth_msg->encoding == enc::TYPE_16UC1)
  {
    convert<uint16_t>(depth_msg, cloud_msg);
  }
  else if (depth_msg->encoding == enc::TYPE_32FC1)
  {
    convert<float>(depth_msg, cloud_msg);
  }
  else
  {
    NODELET_ERROR_THROTTLE(5, "Depth image has unsupported encoding [%s]", depth_msg->encoding.c_str());
    return;
  }

  pub_point_cloud_.publish (cloud_msg);
}

template<typename T>
void PointCloudXyzNodelet::convert(const sensor_msgs::ImageConstPtr& depth_msg,
                                   const PointCloud::Ptr& cloud_msg)
{

  float depth_cx = depth_model_.cx();
  float depth_cy = depth_model_.cy();
  float depth_fx = depth_model_.fx();
  float depth_fy = depth_model_.fy();
  
  const T* depth_row = reinterpret_cast<const T*>(&depth_msg->data[0]);
  int row_step = depth_msg->step / sizeof(T);

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

  float bad_point = std::numeric_limits<float>::quiet_NaN();

  // Use depth image time stamp
  cloud_msg->header.stamp = depth_msg->header.stamp;
  cloud_msg->header.frame_id = depth_msg->header.frame_id;
  cloud_msg->height = 1;
  cloud_msg->width  = pcl_width;
  cloud_msg->is_dense = false;
  cloud_msg->is_bigendian = false;

  NODELET_DEBUG("pcl_width - %d", pcl_width);

  sensor_msgs::PointCloud2Modifier pcd_modifier(*cloud_msg);
  pcd_modifier.setPointCloud2FieldsByString(1, "xyz");

  sensor_msgs::PointCloud2Iterator<float> iter_x(*cloud_msg, "x");
  sensor_msgs::PointCloud2Iterator<float> iter_y(*cloud_msg, "y");
  sensor_msgs::PointCloud2Iterator<float> iter_z(*cloud_msg, "z");

  for (int v = 0; v < int(depth_msg->height); ++v, depth_row += row_step)
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

          ++iter_x, ++iter_y, ++iter_z;
        }
      }
      else
      {
        auto depth_in_meters = depth_row[u];

        float x = depth_in_meters * (u - depth_cx) / depth_fx;
        float y = depth_in_meters * (v - depth_cy) / depth_fy;
        float z = depth_in_meters;

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

        ++iter_x, ++iter_y, ++iter_z;
      }  
    }
  }  
}
};

#include <pluginlib/class_list_macros.h>

PLUGINLIB_EXPORT_CLASS(structure_core::PointCloudXyzNodelet, nodelet::Nodelet)
