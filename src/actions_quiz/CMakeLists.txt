cmake_minimum_required(VERSION 3.8)
project(actions_quiz)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(leo_description REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(actions_quiz_msg REQUIRED) 
find_package(action_msgs REQUIRED)

#add sources 
add_executable(action_client_node src/action_client)
ament_target_dependencies(action_client_node 
	rclcpp
	rclcpp_action
	actions_quiz_msg
)

add_executable(action_server_node src/action_server)
ament_target_dependencies(action_server_node
	rclcpp
	rclcpp_action
	actions_quiz_msg
	leo_description
	geometry_msgs
	nav2_msgs
	tf2
	tf2_geometry_msgs
)

install(TARGETS 
	action_client_node
	action_server_node
  DESTINATION lib/${PROJECT_NAME})

# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}/)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
