#include "actions_quiz_msg/action/distance.hpp"
#include "rclcpp/create_client.hpp"
#include "rclcpp/executors.hpp"
#include "rclcpp/logging.hpp"
#include "rclcpp/node.hpp"
#include "rclcpp_action/client.hpp"
#include "rclcpp_action/client_goal_handle.hpp"
#include "rclcpp_action/create_client.hpp"
#include "rclcpp_action/types.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>

class Action : public rclcpp::Node {
private:
  const std::string action_name = "/distance_as";
  rclcpp_action::Client<actions_quiz_msg::action::Distance>::SharedPtr
      action_client_;
  rclcpp_action::ClientGoalHandle<actions_quiz_msg::action::Distance>::SharedPtr
      current_gol_handle;

public:
  Action() : Node("action_client_node") {
    action_client_ =
        rclcpp_action::create_client<actions_quiz_msg::action::Distance>(
            this, action_name);
    RCLCPP_INFO(this->get_logger(), "Action initialized ...");
  }

  bool send_gool(double x, double y, double yaw) {
    // before dispatching, check server availability
    if (!this->action_client_->wait_for_action_server(
            std::chrono::seconds(10))) {
      RCLCPP_ERROR(this->get_logger(),
                   "Action server /distance_as not available");
      return false;
    }

    // create goal based on action interface
    actions_quiz_msg::action::Distance::Goal goal;

    goal.x = x;
    goal.y = y;
    goal.yaw = yaw;

    // create the current goal
    auto send_goal_options = rclcpp_action::Client<
        actions_quiz_msg::action::Distance>::SendGoalOptions();

    // bind the three callbacks, goal, feedback and result in send_goal_options
    // object;

    send_goal_options.goal_response_callback =
        std::bind(&Action::goal_response_callback, this, std::placeholders::_1);
    send_goal_options.feedback_callback =
        std::bind(&Action::feedback_callback, this, std::placeholders::_1,
                  std::placeholders::_2);
    send_goal_options.result_callback =
        std::bind(&Action::result_callback, this, std::placeholders::_1);

    // always reset the current goal
    current_gol_handle.reset();

    // send the current goal
    action_client_->async_send_goal(goal, send_goal_options);

    return true;
  }

private:
  // callbacks implementation
  // -------------------------------------------------------------------------------------------

  // receives an object of type ClientGoalHandle
  void goal_response_callback(
      const rclcpp_action::ClientGoalHandle<
          actions_quiz_msg::action::Distance>::SharedPtr &gol_handle) {
    // check if goal was not rejected
    if (!gol_handle) {
      RCLCPP_ERROR(get_logger(), "Goal was rejected by the action server");
      return;
    }

    RCLCPP_INFO(get_logger(), "Goal Accepted()");
  }

  // this callback expects two paramaters by default, a goal handle share
  // pointer a shares pointer to the const feedback message, the scope of
  // Distance must be the Feeback defined in the action file (GOAL, RESULT,
  // DISTANCE)
  void feedback_callback(
      rclcpp_action::ClientGoalHandle<
          actions_quiz_msg::action::Distance>::SharedPtr,
      const std::shared_ptr<const actions_quiz_msg::action::Distance::Feedback>
          feedback)

  {
    RCLCPP_INFO(this->get_logger(), "Current distance: %.2f",
                feedback->distance_left);
  }

  void
  result_callback(const rclcpp_action::ClientGoalHandle<
                  actions_quiz_msg::action::Distance>::WrappedResult &result) {

    switch (result.code) {
    case rclcpp_action::ResultCode::SUCCEEDED:
      break;
    case rclcpp_action::ResultCode::ABORTED:
      RCLCPP_ERROR(this->get_logger(), "Goal was aborted");
      return;
    case rclcpp_action::ResultCode::CANCELED:
      RCLCPP_ERROR(this->get_logger(), "Goal was canceled");
      return;
    default:
      RCLCPP_ERROR(this->get_logger(), "Unknown result code");
      return;
    }

    RCLCPP_INFO(this->get_logger(), "Result: %s, Distance traveled: %.2f",
                result.result->success ? "true" : "false",
                result.result->distance_traveled);

    rclcpp::shutdown();
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto client_node = std::make_shared<Action>();
  if (!client_node->send_gool(-8.3, -2.2, -0.2)) {
    RCLCPP_ERROR(client_node->get_logger(), "Could not send the goal ...");
    rclcpp::shutdown();
  }
  rclcpp::spin(client_node);
  rclcpp::shutdown();

  return 0;
}