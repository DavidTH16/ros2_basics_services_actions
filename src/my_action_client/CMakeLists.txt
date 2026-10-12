cmake_minimum_required(VERSION 3.8)
project(my_action_client)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(leo_description REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(action_msgs REQUIRED)
# add the new custom interface
find_package(interface_pkg REQUIRED)

add_executable(action_client_node src/action_client.cpp)
ament_target_dependencies(action_client_node
  "rclcpp"
  "rclcpp_action"
  "leo_description")

add_executable(custom_action_client_node src/custom_action_client)
ament_target_dependencies(custom_action_client_node
	rclcpp
	rclcpp_action
	interface_pkg
)


install(TARGETS 
	action_client_node
	custom_action_client_node
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}/)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()