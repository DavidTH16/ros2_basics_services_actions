cmake_minimum_required(VERSION 3.8)
project(my_action_server)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(leo_description REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(action_msgs REQUIRED)
find_package(interface_pkg REQUIRED)

add_executable(action_server src/action_server.cpp)
ament_target_dependencies(action_server
  "rclcpp"
  "rclcpp_action"
  "leo_description"
  "geometry_msgs"
  "nav2_msgs"
  "tf2"
  "tf2_geometry_msgs"
  "action_msgs")

add_executable(custom_action_server_node src/custom_action_server)
ament_target_dependencies(custom_action_server_node
	rclcpp
	rclcpp_action
	interface_pkg
	geometry_msgs
	nav2_msgs
	tf2
	tf2_geometry_msgs
	action_msgs
)

install(TARGETS 
	action_server
	custom_action_server_node
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()