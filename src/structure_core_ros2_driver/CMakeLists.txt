cmake_minimum_required(VERSION 3.8)
project(structure_core_ros2_driver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-pedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Structure Core SDK
if(NOT SCSDK_ROOT)
  if(DEFINED ENV{SCSDK_ROOT} AND EXISTS "$ENV{SCSDK_ROOT}/Libraries/Structure/Headers")
    set(SCSDK_ROOT $ENV{SCSDK_ROOT})
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/structure_sdk/Libraries/Structure/Headers")
    set(SCSDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/structure_sdk")
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../structure_sdk/Libraries/Structure/Headers")
    set(SCSDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../structure_sdk")
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../structure_ws/src/structure_sdk/Libraries/Structure/Headers")
    set(SCSDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../structure_ws/src/structure_sdk")
  elseif(EXISTS "/media/juza/8AE62852E62840B5/PC/Home/structure_ws/src/structure_sdk/Libraries/Structure/Headers")
    set(SCSDK_ROOT "/media/juza/8AE62852E62840B5/PC/Home/structure_ws/src/structure_sdk")
  endif()
endif()

if(NOT SCSDK_ROOT)
  message(FATAL_ERROR "Could not find Structure Core SDK. Please set SCSDK_ROOT.")
endif()

message(STATUS "Found Structure SDK at: ${SCSDK_ROOT}")

if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(SCSDK_TARGET_ARCH x86_64)
else()
  set(SCSDK_TARGET_ARCH arm64)
endif()

set(SCSDK_STRUCTURE_HEADERS ${SCSDK_ROOT}/Libraries/Structure/Headers)
set(SCSDK_STRUCTURE_LIBDIR ${SCSDK_ROOT}/Libraries/Structure/Linux/${SCSDK_TARGET_ARCH})
set(SCSDK_STRUCTURE_LIB ${SCSDK_STRUCTURE_LIBDIR}/libStructure.so)

if(NOT EXISTS ${SCSDK_STRUCTURE_LIB})
  message(FATAL_ERROR "Structure library not found at: ${SCSDK_STRUCTURE_LIB}")
endif()

# Find ROS 2 packages
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

# Component library
add_library(driver_component SHARED
  src/SCDriver.cpp
)
target_include_directories(driver_component PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${SCSDK_STRUCTURE_HEADERS}>
  $<INSTALL_INTERFACE:include>
)
target_compile_definitions(driver_component PRIVATE "COMPOSITION_BUILDING_DLL")
target_link_libraries(driver_component ${SCSDK_STRUCTURE_LIB})
ament_target_dependencies(driver_component
  rclcpp
  rclcpp_components
  sensor_msgs
  geometry_msgs
  std_msgs
  image_transport
  tf2
  tf2_ros
  tf2_geometry_msgs
)

# Register node component with ROS 2 component manager
rclcpp_components_register_nodes(driver_component "structure_core::Driver")

# Executable wrapper node
add_executable(sc_wrapper_node src/SCWrapper.cpp)
target_include_directories(sc_wrapper_node PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${SCSDK_STRUCTURE_HEADERS}
)
target_link_libraries(sc_wrapper_node driver_component ${SCSDK_STRUCTURE_LIB})
ament_target_dependencies(sc_wrapper_node
  rclcpp
)

# Install targets
install(
  TARGETS driver_component
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  TARGETS sc_wrapper_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

# Install Structure library so it is in runtime linker path
install(
  FILES ${SCSDK_STRUCTURE_LIB}
  DESTINATION lib
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  DIRECTORY launch cfg rviz scripts
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_libraries(driver_component)
ament_export_targets(export_${PROJECT_NAME})
ament_export_dependencies(
  rclcpp
  rclcpp_components
  sensor_msgs
  geometry_msgs
  std_msgs
  image_transport
  tf2
  tf2_ros
  tf2_geometry_msgs
)

ament_package()
