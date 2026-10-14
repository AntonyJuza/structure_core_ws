#include(${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt)

#if(${ROS_VERSION} STREQUAL "2")

cmake_minimum_required(VERSION 3.5)
project(structure_core_ros2_driver)

get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(SCSDK_ROOT ${PARENT_DIR}/StructureSDK-CrossPlatform-0.9)


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
#set(CMAKE_CXX_STANDARD 11)
#add_compile_options(-std=c++11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()


find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rmw_implementation REQUIRED)
#find_package(rclcpp_components REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

###########
## Build ##
###########

include_directories(
                    include
                    ${rclcpp_INCLUDE_DIRS}
                    ${rmw_implementation_INCLUDE_DIRS}
                    ${std_msgs_INCLUDE_DIRS}
                    ${image_transport_INCLUDE_DIRS}
                    ${geometry_msgs_INCLUDE_DIRS}
                    ${tf2_INCLUDE_DIRS}
                    ${tf2_ros_INCLUDE_DIRS}
                    ${tf2_INCLUDE_DIRS}
                    ${tf2_geometry_msgs_INCLUDE_DIRS}
                    ${SCSDK_STRUCTURE_HEADERS})

#set(LINK_LIBRARIES
#    ${catkin_LIBRARIES}
#    ${SCSDK_STRUCTURE_LIB})

set(LINK_LIBRARIES
    ${SCSDK_STRUCTURE_LIB})

# create ament index resource which references the libraries in the binary dir
set(node_plugins "")

add_library(driver_component SHARED
src/SCDriver.cpp)
target_compile_definitions(driver_component
PRIVATE "COMPOSITION_BUILDING_DLL")
target_link_libraries(driver_component
 ${LINK_LIBRARIES})
 ament_target_dependencies(driver_component
  rclcpp
  std_msgs
  sensor_msgs
  stereo_msgs
  image_transport
  tf2
  tf2_ros
  tf2_geometry_msgs
)

rclcpp_register_node_plugins(driver_component "structure_core::Driver")
set(node_plugins "${node_plugins}composition::Driver;$<TARGET_FILE:driver_component>\n")


add_executable(sc_wrapper_node src/SCWrapper.cpp)
target_link_libraries(sc_wrapper_node
    driver_component
)
ament_target_dependencies(sc_wrapper_node
  rclcpp
  class_loader
  driver_component
)

#############
## Install ##
#############

install(
  DIRECTORY include/
  DESTINATION include
)

install(
    TARGETS driver_component
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)


#install(TARGETS driver 
#    RUNTIME DESTINATION lib/${PROJECT_NAME})

install(TARGETS sc_wrapper_node 
    RUNTIME DESTINATION lib/${PROJECT_NAME})

install(
    DIRECTORY launch
    DESTINATION share/${PROJECT_NAME})

install(
    DIRECTORY cfg
    DESTINATION share/${PROJECT_NAME})

install(
    DIRECTORY rviz
    DESTINATION share/${PROJECT_NAME})

ament_package()

#endif()
