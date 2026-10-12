#include "rclcpp/client.hpp"
#include "rclcpp/executors.hpp"
#include "rclcpp/logger.hpp"
#include "rclcpp/logging.hpp"
#include "rclcpp/node_interfaces/get_node_base_interface.hpp"
#include <chrono>
//#include <interface_pkg/srv/TextRecognition.hpp>
#include <interface_pkg/srv/text_recognition.hpp>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <string>

using namespace std::chrono_literals;

class TextRecognitionClientCustom : public rclcpp::Node {
private: // define the client
  rclcpp::Client<interface_pkg::srv::TextRecognition>::SharedPtr client_;

public:
  TextRecognitionClientCustom() : Node("text_recognition_client_custom") {
    std::string service_name = "/text_recognition-service_custom";
    client_ =
        this->create_client<interface_pkg::srv::TextRecognition>(service_name);

    // loop to wait for server
    while (!client_->wait_for_service(1s)) {
      if (!rclcpp::ok()) {
        RCLCPP_ERROR(this->get_logger(),
                     "Interrupted while waitin for the Service");
        return;
      }
      RCLCPP_INFO(this->get_logger(),
                  "Services nor available, waiting again...");
    };
  }

  // method to send the request in this class, pass the label you look for
  void send_request(const std::string &label) {
    auto request =
        std::make_shared<interface_pkg::srv::TextRecognition::Request>();

    // set the label
    request->label = label;

    RCLCPP_INFO(this->get_logger(), "Searching for label: %s", label.c_str());

    // send the request asynchronously
    auto result_futur = client_->async_send_request(request);

    // Wait for the response
    if (rclcpp::spin_until_future_complete(this->get_node_base_interface(),
                                           result_futur) ==
        rclcpp::FutureReturnCode::SUCCESS) {
      auto response = result_futur.get();

      if (response->success) {

        RCLCPP_INFO(this->get_logger(), "Label '%s' found!", label.c_str());
        RCLCPP_INFO(this->get_logger(), "Bounding Box: (%d, %d) -> (%d, %d)",
                    response->start_x, response->start_y, response->end_x,
                    response->end_y);

      } else {

        RCLCPP_INFO(this->get_logger(), "Label '%s' not found", label.c_str());
        RCLCPP_INFO(this->get_logger(), "Bounding Box: (%d, %d) -> (%d, %d)",
                    response->start_x, response->start_y, response->end_x,
                    response->end_y);
      }

    } else {

      RCLCPP_ERROR(this->get_logger(), "Failed to call Service");
    }
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto client = std::make_shared<TextRecognitionClientCustom>();
  std::string label_to_find = "Food";
  client->send_request(label_to_find);
  rclcpp::shutdown();
  return 0;
}