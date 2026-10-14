cmake_minimum_required(VERSION 2.8.3)
project(structure_core_ros_driver)

get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(SCSDK_ROOT ${PARENT_DIR}/../)

set(BUILD_EXAMPLES "FALSE")

if( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
	set(SCSDK_TARGET_ARCH x86_64)
else()
	set(SCSDK_TARGET_ARCH arm64)
endif()

if(NOT SCSDK_TARGET_ARCH)
    message(WARNING "SCSDK_TARGET_ARCH is not set (build scripts will do this automatically). Assuming x86_64.")
    set(SCSDK_TARGET_ARCH x86_64)
endif()

set(SCSDK_STRUCTURE_HEADERS ${SCSDK_ROOT}/Libraries/Structure/Headers)
set(SCSDK_STRUCTURE_LIBDIR ${SCSDK_ROOT}/Libraries/Structure/Linux/${SCSDK_TARGET_ARCH})
set(SCSDK_STRUCTURE_LIB ${SCSDK_STRUCTURE_LIBDIR}/libStructure.so)

## Compile as C++11, supported in ROS Kinetic and newer
set(CMAKE_CXX_STANDARD 11)
add_compile_options(-std=c++11)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages

if( ${BUILD_EXAMPLES} STREQUAL "TRUE" )
  find_package(OpenCV REQUIRED)
  find_package(catkin REQUIRED COMPONENTS
  dynamic_reconfigure
  roscpp
  rospy
  std_msgs
  nodelet
  image_transport
  image_geometry
  tf2_geometry_msgs
  tf2_ros 
  cv_bridge
  pcl_ros
  pcl_conversions
  )
else()
find_package(catkin REQUIRED COMPONENTS
  dynamic_reconfigure
  roscpp
  rospy
  std_msgs
  nodelet
  image_transport
  tf2_geometry_msgs
  tf2_ros
  image_geometry
  cv_bridge
) 
endif()

## System dependencies are found with CMake's conventions
# find_package(Boost REQUIRED COMPONENTS system)

## Uncomment this if the package has a setup.py. This macro ensures
## modules and global scripts declared therein get installed
## See http://ros.org/doc/api/catkin/html/user_guide/setup_dot_py.html
# catkin_python_setup()

################################################
## Declare ROS messages, services and actions ##
################################################

## To declare and build messages, services or actions from within this
## package, follow these steps:
## * Let MSG_DEP_SET be the set of packages whose message types you use in
##   your messages/services/actions (e.g. std_msgs, actionlib_msgs, ...).
## * In the file package.xml:
##   * add a build_depend tag for "message_generation"
##   * add a build_depend and a exec_depend tag for each package in MSG_DEP_SET
##   * If MSG_DEP_SET isn't empty the following dependency has been pulled in
##     but can be declared for certainty nonetheless:
##     * add a exec_depend tag for "message_runtime"
## * In this file (CMakeLists.txt):
##   * add "message_generation" and every package in MSG_DEP_SET to
##     find_package(catkin REQUIRED COMPONENTS ...)
##   * add "message_runtime" and every package in MSG_DEP_SET to
##     catkin_package(CATKIN_DEPENDS ...)
##   * uncomment the add_*_files sections below as needed
##     and list every .msg/.srv/.action file to be processed
##   * uncomment the generate_messages entry below
##   * add every package in MSG_DEP_SET to generate_messages(DEPENDENCIES ...)

## Generate messages in the 'msg' folder
# add_message_files(
#   FILES
#   Message1.msg
#   Message2.msg
# )

## Generate services in the 'srv' folder
# add_service_files(
#   FILES
#   Service1.srv
#   Service2.srv
# )

## Generate actions in the 'action' folder
# add_action_files(
#   FILES
#   Action1.action
#   Action2.action
# )

## Generate added messages and services with any dependencies listed here
# generate_messages(
#   DEPENDENCIES
#   std_msgs
# )

################################################
## Declare ROS dynamic reconfigure parameters ##
################################################

## To declare and build dynamic reconfigure parameters within this
## package, follow these steps:
## * In the file package.xml:
##   * add a build_depend and a exec_depend tag for "dynamic_reconfigure"
## * In this file (CMakeLists.txt):
##   * add "dynamic_reconfigure" to
##     find_package(catkin REQUIRED COMPONENTS ...)
##   * uncomment the "generate_dynamic_reconfigure_options" section below
##     and list every .cfg file to be processed

## Generate dynamic reconfigure parameters in the 'cfg' folder
generate_dynamic_reconfigure_options(
   cfg/SCParams.cfg
)

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if your package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need

if( ${BUILD_EXAMPLES} STREQUAL "TRUE" )
  catkin_package(
  CATKIN_DEPENDS
  roscpp
  rosconsole
  geometry_msgs
  rosbag
  nodelet
  cv_bridge
  image_transport
  tf2_geometry_msgs
  tf2_ros
  cv_bridge
  pcl_ros
  pcl_conversions
)
else()
catkin_package(
  CATKIN_DEPENDS
  roscpp
  rosconsole
  geometry_msgs
  rosbag
  nodelet
  cv_bridge
  image_transport
  tf2_geometry_msgs
  tf2_ros
)
endif()

set(NODE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/SCWrapper.cpp)
set(NODELET_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/SCDriver.cpp)
set(NODELET_RGBD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/SCRGBDPointCloud.cpp)
set(NODELET_D_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/SCDepthPointCloud.cpp)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
include_directories(
  ${SCSDK_STRUCTURE_HEADERS}
  include
  ${catkin_INCLUDE_DIRS}
)

set(LINK_LIBRARIES
    ${catkin_LIBRARIES}
    ${SCSDK_STRUCTURE_LIB})

## Add cmake target dependencies of the library
## as an example, code may need to be generated before libraries
## either from message generation or dynamic reconfigure
# add_dependencies(${PROJECT_NAME} ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

add_library(SCWrapper  ${NODELET_SRC} ${NODELET_RGBD_SRC} ${NODELET_D_SRC})
target_link_libraries(SCWrapper ${LINK_LIBRARIES})
add_dependencies(SCWrapper ${PROJECT_NAME}_gencfg  ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

add_executable(sc_wrapper_node ${NODE_SRC})
target_link_libraries(sc_wrapper_node SCWrapper ${LINK_LIBRARIES})
add_dependencies(sc_wrapper_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${PROJECT_NAME}_gencfg)

if( ${BUILD_EXAMPLES} STREQUAL "TRUE" )
  add_executable(opencv_subscriber examples/opencvSubscriber.cpp)
  target_link_libraries(opencv_subscriber ${catkin_LIBRARIES} ${OpenCV_LIBS})
  #add_dependencies(listener beginner_tutorials_generate_messages_cpp)

  add_executable(pcl_subscriber examples/pclSubscriber.cpp)
  target_link_libraries(pcl_subscriber ${catkin_LIBRARIES})
endif()

## Declare a C++ executable
## With catkin_make all packages are built within a single CMake context
## The recommended prefix ensures that target names across packages don't collide
# add_executable(${PROJECT_NAME}_node src/beginner_tutorials_node.cpp)
#  add_executable(driver src/SCDriver.cpp)	

## Rename C++ executable without prefix
## The above recommended prefix causes long target names, the following renames the
## target back to the shorter version for ease of user use
## e.g. "rosrun someones_pkg node" instead of "rosrun someones_pkg someones_pkg_node"
# set_target_properties(${PROJECT_NAME}_node PROPERTIES OUTPUT_NAME node PREFIX "")

## Add cmake target dependencies of the executable
## same as for the library above
# add_dependencies(${PROJECT_NAME}_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Specify libraries to link a library or executable target against
# target_link_libraries(${PROJECT_NAME}_node
#   ${catkin_LIBRARIES}
# )

#target_link_libraries(driver
#	${LINK_LIBRARIES})

#############
## Install ##
#############

# all install targets should use catkin DESTINATION variables
# See http://ros.org/doc/api/catkin/html/adv_user_guide/variables.html

## Mark executable scripts (Python etc.) for installation
## in contrast to setup.py, you can choose the destination
# install(PROGRAMS
#   scripts/my_python_script
#   DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
# )

## Mark executables and/or libraries for installation
# install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_node
#   ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
# )

## Mark cpp header files for installation
# install(DIRECTORY include/${PROJECT_NAME}/
#   DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
#   FILES_MATCHING PATTERN "*.h"
#   PATTERN ".svn" EXCLUDE
# )

## Mark other files for installation (e.g. launch and bag files, etc.)
# install(FILES
#   # myfile1
#   # myfile2
#   DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
# )


install(TARGETS
  SCWrapper
  sc_wrapper_node
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
  
install(FILES
  nodelet_plugins.xml
DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

install(DIRECTORY
  launch
DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

#############
## Testing ##
#############

## Add gtest based cpp test target and link libraries
#catkin_add_gtest(${PROJECT_NAME}-test src/test/SCPublishersTests.cpp)
#if(TARGET ${PROJECT_NAME}-test)
#   target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
#endif()

## Add folders to be run by python nosetests
# catkin_add_nosetests(test)
