#include "rclcpp/duration.hpp"
#include "rclcpp/logging.hpp"
#include "rclcpp/publisher.hpp"
#include "rclcpp/service.hpp"
#include <chrono>
#include <geometry_msgs/msg/twist.hpp>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <services_quiz_srv/srv/turn.hpp>

using namespace std::chrono_literals;

class Server : public rclcpp::Node {

private:
  rclcpp::Service<services_quiz_srv::srv::Turn>::SharedPtr server_ptr_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_cmd_;
  float time, ang_velocity;
  std::string turn_robot;

public:
  Server() : Node("server_node") {

    // create the service
    std::string name_service = "/turn";
    server_ptr_ = this->create_service<services_quiz_srv::srv::Turn>(
        name_service, std::bind(&Server::server_callback, this,
                                std::placeholders::_1, std::placeholders::_2));

    // create the publisher
    publisher_cmd_ =
        this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);

    RCLCPP_INFO(this->get_logger(), "Custom service initialized...");
  }

private:
  // callback implementation
  void server_callback(
      const std::shared_ptr<services_quiz_srv::srv::Turn::Request> request,
      std::shared_ptr<services_quiz_srv::srv::Turn::Response> response) {

    turn_robot = request->turn;

    if (turn_robot == "right") {
      timer();
      response->success = true;
    } else if (turn_robot == "left") {
      timer();
      response->success = true;
    } else {
      RCLCPP_INFO(this->get_logger(), "Direction not found...");
      response->success = false;
    }
  }

  // method to implement timer
  void timer() {
    // twist message
    geometry_msgs::msg::Twist velocity;
    rclcpp::Duration duration = 10s;

    auto start_time = this->now();
    while ((this->now() - start_time) < duration) {
      if (turn_robot == "right") {
        velocity.angular.z = -0.2;
        publisher_cmd_->publish(velocity);
      } else {
        velocity.angular.z = 0.2;
        publisher_cmd_->publish(velocity);
      }
    }
    velocity.angular.z = 0.0;
    publisher_cmd_->publish(velocity);
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto server_node = std::make_shared<Server>();
  rclcpp::spin(server_node);
  rclcpp::shutdown();
  return 0;
}