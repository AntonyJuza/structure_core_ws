#include "ros/ros.h"
#include "std_msgs/String.h"
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <image_transport/image_transport.h>


static const std::string COLOR_WINDOW = "Color window";
static const std::string DEPTH_WINDOW = "Depth window";
 
class SCImageConverter
{
  ros::NodeHandle nh_;
  image_transport::ImageTransport color_it_;
  image_transport::ImageTransport depth_it_;
  image_transport::Subscriber color_sub_;
  image_transport::Subscriber depth_sub_;
  image_transport::Publisher image_pub_;

public:
  SCImageConverter()
    : color_it_(nh_), depth_it_(nh_)
  {
    // crete subscribers for color and depth images from Structure Core
    color_sub_ = color_it_.subscribe("/sc/rgb/image", 1, &SCImageConverter::colorImageCb, this);
    depth_sub_ = depth_it_.subscribe("/sc/depth/image", 1, &SCImageConverter::depthImageCb, this);
 
   cv::namedWindow(COLOR_WINDOW);
  }

  ~SCImageConverter()
  {
    cv::destroyWindow(COLOR_WINDOW);
    cv::destroyWindow(DEPTH_WINDOW);
  }

  void colorImageCb(const sensor_msgs::ImageConstPtr& msg)
  {
    cv_bridge::CvImagePtr cv_ptr;
    try
    {
      // use cv_bridge to conver ROS message to OpenCV format
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    }
    catch (cv_bridge::Exception& e)
    {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }
   
      // Update GUI Window
      cv::imshow(COLOR_WINDOW, cv_ptr->image);
      cv::waitKey(3);

  }

  void depthImageCb(const sensor_msgs::ImageConstPtr& msg)
  {
    cv_bridge::CvImagePtr cv_ptr;
    try
    {
      // use cv_bridge to conver ROS message to OpenCV format
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::TYPE_32FC1);
    }
    catch (cv_bridge::Exception& e)
    {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }
   
      // Update GUI Window
      cv::imshow(DEPTH_WINDOW, cv_ptr->image);
      cv::waitKey(3);

  }

};
   
int main(int argc, char** argv)
{
  // run ROS node
  ros::init(argc, argv, "image_viewer");
  SCImageConverter ic;
  ros::spin();
  
  return 0;
   }
