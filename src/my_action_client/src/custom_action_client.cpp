#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "interface_pkg/action/go_to_pose.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

class CustomActionClient : public rclcpp::Node {
public:
  using GoToPose = interface_pkg::action::GoToPose;
  using GoalHandleGoToPose = rclcpp_action::ClientGoalHandle<GoToPose>;

  explicit CustomActionClient(const rclcpp::NodeOptions &options)
      : Node("custom_action_client", options) {
    action_client_ = rclcpp_action::create_client<GoToPose>(this, action_name_);

    timeout_timer_ = create_wall_timer(
        std::chrono::milliseconds(100),
        std::bind(&CustomActionClient::check_goal_timeout, this));

    RCLCPP_INFO(get_logger(), "Custom action client started.");

    RCLCPP_INFO(get_logger(), "Goal timeout: %.1f seconds",
                goal_timeout_seconds_);
  }

  bool send_goal(double x, double y, double yaw) {
    using namespace std::placeholders;

    if (!rclcpp::ok()) {
      return false;
    }

    RCLCPP_INFO(get_logger(), "Waiting for action server '%s'...",
                action_name_.c_str());

    if (!action_client_->wait_for_action_server(std::chrono::seconds(10))) {
      if (rclcpp::ok()) {
        RCLCPP_ERROR(get_logger(),
                     "Action server '%s' was not available "
                     "after 10 seconds.",
                     action_name_.c_str());
      }

      return false;
    }

    GoToPose::Goal goal_msg;
    goal_msg.x = x;
    goal_msg.y = y;
    goal_msg.yaw = yaw;

    RCLCPP_INFO(get_logger(), "Sending goal: x=%.2f, y=%.2f, yaw=%.2f", x, y,
                yaw);

    auto send_goal_options = rclcpp_action::Client<GoToPose>::SendGoalOptions();

    send_goal_options.goal_response_callback =
        std::bind(&CustomActionClient::goal_response_callback, this, _1);

    send_goal_options.feedback_callback =
        std::bind(&CustomActionClient::feedback_callback, this, _1, _2);

    send_goal_options.result_callback =
        std::bind(&CustomActionClient::result_callback, this, _1);

    current_goal_handle_.reset();
    goal_active_ = false;
    timeout_triggered_ = false;

    action_client_->async_send_goal(goal_msg, send_goal_options);

    return true;
  }

private:
  const std::string action_name_ = "/go_to_pose";

  // Maximum amount of time allowed for the goal.
  const double goal_timeout_seconds_ = 30.0;

  rclcpp_action::Client<GoToPose>::SharedPtr action_client_;

  rclcpp::TimerBase::SharedPtr timeout_timer_;

  GoalHandleGoToPose::SharedPtr current_goal_handle_;

  rclcpp::Time goal_start_time_;

  bool goal_active_ = false;
  bool timeout_triggered_ = false;

  void
  goal_response_callback(const GoalHandleGoToPose::SharedPtr &goal_handle) {
    if (!goal_handle) {
      RCLCPP_ERROR(get_logger(), "Goal was rejected by the action server.");

      goal_active_ = false;
      return;
    }

    current_goal_handle_ = goal_handle;
    goal_start_time_ = now();
    goal_active_ = true;
    timeout_triggered_ = false;

    RCLCPP_INFO(get_logger(), "Goal accepted. Timeout starts now.");
  }

  void
  feedback_callback(GoalHandleGoToPose::SharedPtr,
                    const std::shared_ptr<const GoToPose::Feedback> feedback) {
    (void)feedback;
  }

  void result_callback(const GoalHandleGoToPose::WrappedResult &result) {
    goal_active_ = false;
    current_goal_handle_.reset();

    switch (result.code) {
    case rclcpp_action::ResultCode::SUCCEEDED:
      RCLCPP_INFO(get_logger(), "The robot reached the goal.");
      break;

    case rclcpp_action::ResultCode::ABORTED:
      RCLCPP_WARN(get_logger(), "The action server aborted the goal.");
      break;

    case rclcpp_action::ResultCode::CANCELED:
      if (timeout_triggered_) {
        RCLCPP_WARN(get_logger(),
                    "The goal was canceled because the timeout expired.");
      } else {
        RCLCPP_WARN(get_logger(), "The goal was canceled.");
      }
      break;

    default:
      RCLCPP_ERROR(get_logger(), "Unknown action result.");
      break;
    }
  }

  void check_goal_timeout() {
    if (!goal_active_ || timeout_triggered_ || !current_goal_handle_) {
      return;
    }

    const double elapsed_seconds = (now() - goal_start_time_).seconds();

    if (elapsed_seconds < goal_timeout_seconds_) {
      return;
    }

    timeout_triggered_ = true;
    goal_active_ = false;

    RCLCPP_WARN(get_logger(),
                "The robot did not reach the goal within %.1f seconds.",
                goal_timeout_seconds_);

    RCLCPP_WARN(get_logger(), "Requesting cancellation of the current goal.");

    action_client_->async_cancel_goal(current_goal_handle_);
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);

  auto action_client =
      std::make_shared<CustomActionClient>(rclcpp::NodeOptions());

  if (!action_client->send_goal(-8.0, 6.0, 1.57)) {
    RCLCPP_ERROR(action_client->get_logger(), "Could not send the goal.");

    rclcpp::shutdown();
    return 1;
  }

  rclcpp::spin(action_client);

  rclcpp::shutdown();
  return 0;
}