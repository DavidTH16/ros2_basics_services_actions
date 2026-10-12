#include "actions_quiz_msg/action/distance.hpp"
#include "geometry_msgs/msg/detail/pose_with_covariance__struct.hpp"
#include "geometry_msgs/msg/detail/pose_with_covariance_stamped__struct.hpp"
#include "geometry_msgs/msg/pose_with_covariance.hpp"
#include "leo_description/action/go_to_pose.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "nav_msgs/msg/detail/odometry__struct.hpp"
#include "rclcpp/create_publisher.hpp"
#include "rclcpp/logging.hpp"
#include "rclcpp/node.hpp"
#include "rclcpp/publisher.hpp"
#include "rclcpp/subscription.hpp"
#include "rclcpp_action/client.hpp"
#include "rclcpp_action/client_goal_handle.hpp"
#include "rclcpp_action/create_client.hpp"
#include "rclcpp_action/create_server.hpp"
#include "rclcpp_action/server.hpp"
#include "rclcpp_action/server_goal_handle.hpp"
#include "rclcpp_action/types.hpp"
#include "std_msgs/msg/detail/float64__struct.hpp"
#include "std_msgs/msg/float64.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <future>
#include <memory>
#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <thread>

class Server : public rclcpp::Node {
  // alliases to improve readilibity, create shorter workspaces
  // alias to go to pose, for distance, for navigations
public:
  using GoToPose = leo_description::action::GoToPose;
  using Distance = actions_quiz_msg::action::Distance;
  using GoalHandleDistance = rclcpp_action::ServerGoalHandle<Distance>;
  using NavigateToPose = nav2_msgs::action::NavigateToPose;
  using GoalhandleNavigation = rclcpp_action::ClientGoalHandle<NavigateToPose>;

private:
  std::atomic<double> x_init, y_init;
  rclcpp_action::Server<Distance>::SharedPtr action_server_;
  rclcpp_action::Client<NavigateToPose>::SharedPtr nav_to_pose_client_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr
      initial_pose_publisher_;
  rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr
      odom_subcriber_;
  // required publishers for grading
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr publisher_distance_left;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr
      publisher_distance_traveled;

public:
  // initialie the constructor, create the service and the callbacks
  // to handle the goal
  Server() : Node("server_node") {
    // create the server
    this->action_server_ = rclcpp_action::create_server<Distance>(
        this, "/distance_as",
        std::bind(&Server::handle_goal, this, std::placeholders::_1,
                  std::placeholders::_2),
        std::bind(&Server::handle_cancel, this, std::placeholders::_1),
        std::bind(&Server::handle_accepted, this, std::placeholders::_1));

    // create the client for navigation
    this->nav_to_pose_client_ =
        rclcpp_action::create_client<NavigateToPose>(this, "/navigate_to_pose");

    // create the publisher for initial pose
    initial_pose_publisher_ =
        this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
            "/initialpose", 10);

    // create subscriber for odometry
    odom_subcriber_ = this->create_subscription<
        geometry_msgs::msg::PoseWithCovarianceStamped>(
        "/amcl_pose", 10,
        std::bind(&Server::odom_callback, this, std::placeholders::_1));

    // create publisher for distance left;
    publisher_distance_left =
        this->create_publisher<std_msgs::msg::Float64>("/distance_left", 10);

    // create publisher for distance traveled
    publisher_distance_traveled =
        this->create_publisher<std_msgs::msg::Float64>("/distance_traveled",
                                                       10);

    RCLCPP_INFO(this->get_logger(),
                "Server initialized, waiting for request ...");

    // wait for the localization node to be ready
    //wait_for_localization();

    // set initial position
    set_initial_pose(0.0, 0.0, 0.0);
  } 

private:
  // method to wait until the localization node is ready
  //---------------------------------------------------------------------------------
  void wait_for_localization() {
    RCLCPP_INFO(this->get_logger(), "Waiting for localization to be active...");

    // Wait for subscribers to /initialpose
    while (this->count_subscribers("/initialpose") == 0) {
      RCLCPP_INFO(this->get_logger(),
                  "Waiting for subscribers to /initialpose...");
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Wait for /amcl_pose topic
    while (!topic_exists("/amcl_pose")) {
      RCLCPP_INFO(this->get_logger(), "Waiting for /amcl_pose topic...");
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    RCLCPP_INFO(this->get_logger(), "Localization is active.");
  }

  // method to check if topic already exist
  //----------------------------------------------------------------------------------
  bool topic_exists(const std::string &topic_name) {
    auto topics = this->get_topic_names_and_types();
    for (const auto &topic : topics) {
      if (topic.first == topic_name) {
        return true;
      }
    }
    return false;
  }

  // method to initialize the initial position
  //----------------------------------------------------------------------------------
  void set_initial_pose(double x, double y, double yaw) {
    auto initial_pose = geometry_msgs::msg::PoseWithCovarianceStamped();
    initial_pose.header.frame_id = "map";
    initial_pose.header.stamp = this->get_clock()->now();
    initial_pose.pose.pose.position.x = x;
    initial_pose.pose.pose.position.y = y;

    // set initial pose
    x_init = x;
    y_init = y;

    // Set the orientation as quaternion (for yaw rotation)
    tf2::Quaternion q;
    q.setRPY(0, 0, yaw);
    initial_pose.pose.pose.orientation = tf2::toMsg(q);
    // Publish the initial pose multiple times
    for (int i = 0; i < 10; ++i) {
      initial_pose_publisher_->publish(initial_pose);
      RCLCPP_INFO(this->get_logger(), "Publishing initial pose (%d/10)", i + 1);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    RCLCPP_INFO(this->get_logger(),
                "Initial pose set to x: %.2f, y: %.2f, yaw: %.2f", x, y, yaw);
  }

  // callback to get data from the odometry
  //-----------------------------------------------------------------------------------
  void odom_callback(
      const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg) {
    x_init = msg->pose.pose.position.x;
    y_init = msg->pose.pose.position.y;
  }

  // callback to handle the goal sent by the user, and accept
  // recieve the GOAL ID, and the user's goal.
  // ----------------------------------------------------------------------------------
  rclcpp_action::GoalResponse
  handle_goal(const rclcpp_action::GoalUUID &uuid,
              std::shared_ptr<const Distance::Goal> goal) {

    (void)uuid; // to ignore at compilation time.

    RCLCPP_INFO(this->get_logger(),
                "Received goal request: X = %.2f, Y = %.2f, YAW = %.2f",
                goal->x, goal->y, goal->yaw);

    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  // callback to handle the cancelation of the goal if it is currently running.
  // receives the user's goal.
  //-----------------------------------------------------------------------------------------
  rclcpp_action::CancelResponse
  handle_cancel(const std::shared_ptr<GoalHandleDistance> goal_handle) {
    RCLCPP_INFO(this->get_logger(), "Received request to cancel goal");

    return rclcpp_action::CancelResponse::ACCEPT;
  }

  // callback to execute the given goal once it was accepeted
  // receives the goalhandle, and execute in another thread using another
  // callback.
  //-----------------------------------------------------------------------------------------
  void handle_accepted(const std::shared_ptr<GoalHandleDistance> goal_handle) {
    std::thread{std::bind(&Server::execute, this, goal_handle)}.detach();
  }

  // method to execute the accepted goal and send the feedback
  //-----------------------------------------------------------------------------------------
  void execute(const std::shared_ptr<GoalHandleDistance> goal_handle) {
    RCLCPP_INFO(this->get_logger(), "Executing goal ...");
    const auto goal = goal_handle->get_goal();
    auto feedback = std::make_shared<Distance::Feedback>();
    auto result = std::make_shared<Distance::Result>();

    bool navigation_result =
        send_navigation_goal(goal->x, goal->y, goal->yaw, goal_handle, result);

    if (navigation_result) {
      RCLCPP_INFO(this->get_logger(), "Goal reached successfully!");
      result->success = true;
      goal_handle->succeed(result);
    } else {
      RCLCPP_INFO(this->get_logger(), "Failed to reach goal :(");
      result->success = false;
      goal_handle->abort(result);
    }
  }

  // compute the distance to the target
  //----------------------------------------------------------------------------------
  double get_odometry(double x, double y) {

    double current_x = x - x_init;
    double current_y = y - y_init;
    auto distance = std::hypot(current_x, current_y);

    return distance;
  }

  // method to send goal into the navigation system client.
  //--------------------------------------------------------------------------------
  bool
  send_navigation_goal(double x, double y, double yaw,
                       const std::shared_ptr<GoalHandleDistance> goal_handle,
                       std::shared_ptr<Distance::Result> result) {
    // handle feedback
    auto feedback = std::make_shared<Distance::Feedback>();

    // send info to respective topics
    auto dist_left = std_msgs::msg::Float64();
    auto dist_total = std_msgs::msg::Float64();

    // set linear goal coordinates
    auto goal_msg = NavigateToPose::Goal();
    goal_msg.pose.pose.position.x = x;
    goal_msg.pose.pose.position.y = y;

    // set angular goal angle
    tf2::Quaternion q;
    q.setRPY(0, 0, yaw);
    goal_msg.pose.pose.orientation = tf2::toMsg(q);

    // set frame and time stamp
    goal_msg.pose.header.frame_id = "map";
    goal_msg.pose.header.stamp = this->get_clock()->now();

    // Wait for the NavigateToPose action server
    RCLCPP_INFO(this->get_logger(),
                "Waiting for the NavigateToPose action server...");
    if (!nav_to_pose_client_->wait_for_action_server(
            std::chrono::seconds(10))) {
      RCLCPP_ERROR(this->get_logger(),
                   "NavigateToPose action server not available");
      return false;
    }

    // send the goal
    RCLCPP_INFO(this->get_logger(),
                "Sending navigation goal to: X = %.2f, Y = %.2f, YAW = %.2f", x,
                y, yaw);

    auto send_goal_future = nav_to_pose_client_->async_send_goal(goal_msg);

    // Wait for goal to be accepted
    if (send_goal_future.wait_for(std::chrono::seconds(10)) !=
        std::future_status::ready) {
      RCLCPP_ERROR(this->get_logger(), "Failed to send goal");
      return false;
    }

    auto nav_goal_handle = send_goal_future.get();
    if (!nav_goal_handle) {
      RCLCPP_INFO(this->get_logger(), "Navigation goal rejected");
      return false;
    }

    RCLCPP_INFO(this->get_logger(), "Navigation goal accepted");

    // Wait for result
    auto get_result_future =
        nav_to_pose_client_->async_get_result(nav_goal_handle);

    // varaibles to get total distance
    double total_distance = 0.0;
    double last_x = x_init.load();
    double last_y = y_init.load();
    // polling loop
    while (rclcpp::ok()) {
      auto status = get_result_future.wait_for(std::chrono::milliseconds(5));
      if (status == std::future_status::ready) {
        break;
      }

      // get current coordinates
      double current_x = x_init.load();
      double current_y = y_init.load();

      // total accumulate distance
      total_distance += std::hypot(current_x - last_x, current_y - last_y);

      // update the coordinates
      last_x = current_x;
      last_y = current_y;

      const auto goal = goal_handle->get_goal();
      double feedback_odometry = get_odometry(goal->x, goal->y);

      // publish the distance left in the topic
      dist_left.data = feedback_odometry;
      publisher_distance_left->publish(dist_left);

      feedback->distance_left = feedback_odometry;
      goal_handle->publish_feedback(feedback);
    }

    auto nav_result = get_result_future.get();
    if (nav_result.code == rclcpp_action::ResultCode::SUCCEEDED) {
      RCLCPP_INFO(this->get_logger(),
                  "Navigation succeeded. Total distance: %.2f", total_distance);

      dist_total.data = total_distance;
      publisher_distance_traveled->publish(dist_total);
      result->distance_traveled = total_distance;
      return true;
    } else {
      RCLCPP_INFO(this->get_logger(), "Navigation failed with status: %d",
                  static_cast<int>(nav_result.code));
      return false;
    }
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto server_node = std::make_shared<Server>();
  rclcpp::spin(server_node);
  rclcpp::shutdown();
  return 0;
}