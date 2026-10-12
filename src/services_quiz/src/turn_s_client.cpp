#include "rclcpp/client.hpp"
#include "rclcpp/executors.hpp"
#include "rclcpp/future_return_code.hpp"
#include "rclcpp/logging.hpp"
#include "rclcpp/node.hpp"
#include "rclcpp/utilities.hpp"
#include <chrono>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <services_quiz_srv/srv/turn.hpp>
#include <string>

using namespace std::chrono_literals;

class Client : public rclcpp::Node {
private:
  rclcpp::Client<services_quiz_srv::srv::Turn>::SharedPtr client_service_;

public:
  Client() : Node("client_node") {
    // create the client service
    std::string service_name = "/turn";
    client_service_ =
        this->create_client<services_quiz_srv::srv::Turn>(service_name);

    // loop to wait for the service
    while (!client_service_->wait_for_service(1s)) {
      if (!rclcpp::ok()) {
        RCLCPP_ERROR(this->get_logger(),
                     "Interrupted while waiting for the service");
        return;
      } else {
        RCLCPP_INFO(this->get_logger(),
                    "Service not available, waiting again...");
      }
    }
  }

  // method to request
  void client_request(const std::string &turn_, float angular_velocity_,
                      float time_) {
    auto request = std::make_shared<services_quiz_srv::srv::Turn::Request>();

    // set the values in the request
    request->turn = turn_;
    request->angular_velocity = angular_velocity_;
    request->time = time_;

    RCLCPP_INFO(
        this->get_logger(),
        "Sending instructions, turn: %s, angular_velocity: %.2f, time: %2.f",
        turn_.c_str(), angular_velocity_, time_);

    // send the request in asynchronous mode
    auto result_future = client_service_->async_send_request(request);

    // wait for the response
    if (rclcpp::spin_until_future_complete(this->get_node_base_interface(),
                                           result_future) ==
        rclcpp::FutureReturnCode::SUCCESS) {
      auto response = result_future.get();
      if (!response) {
        RCLCPP_WARN(this->get_logger(), "Something when wrong");
      }

      RCLCPP_INFO(this->get_logger(), "process has finished cleanly...");

    }

    else {
      RCLCPP_INFO(this->get_logger(), "Error to call the service");
    }
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto client_node = std::make_shared<Client>();
  std::string turn = "right";
  float ang_velocity = 0.2;
  float time = 10.0;
  client_node->client_request(turn, ang_velocity, time);
  rclcpp::shutdown();
  return 0;
}