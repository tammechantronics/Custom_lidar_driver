cmake_minimum_required(VERSION 3.8)
project(custom_lidar_ros2_driver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(custom_lidar_sdk REQUIRED)

add_executable(custom_lidar_ros2_driver_node
  src/custom_lidar_ros2_driver_node.cpp
)

target_compile_features(custom_lidar_ros2_driver_node PUBLIC cxx_std_17)

ament_target_dependencies(custom_lidar_ros2_driver_node
  rclcpp
  sensor_msgs
  std_srvs
)

target_link_libraries(custom_lidar_ros2_driver_node
  custom_lidar_sdk::custom_lidar_sdk
)

install(TARGETS custom_lidar_ros2_driver_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch params rviz
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
