#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <string>
#include <utility>
#include <chrono>

#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

#include <interface_pkg/srv/text_recognition.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/msg/image.hpp>

class TextRecognitionServiceCustom : public rclcpp::Node {
public:
  TextRecognitionServiceCustom()
      : Node("text_recognition_service_custom"), image_received_(false) {
    // Subscribe to the camera image
    image_subscriber_ = this->create_subscription<sensor_msgs::msg::Image>(
        "/leo/camera/image_raw", 1,
        std::bind(&TextRecognitionServiceCustom::image_callback, this,
                  std::placeholders::_1));

    // Create the custom Service Server
    service_ = this->create_service<interface_pkg::srv::TextRecognition>(
        "/text_recognition_service_custom",
        std::bind(
            &TextRecognitionServiceCustom::handle_text_recognition_request,
            this, std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(this->get_logger(),
                "Text Recognition Custom Service Server initialized");
  }

private:
  void image_callback(const sensor_msgs::msg::Image::SharedPtr msg) {
    try {
      auto cv_ptr =
          cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);

      // Store the latest camera image
      latest_image_ = cv_ptr->image.clone();
      image_received_ = true;

    } catch (const cv_bridge::Exception &e) {
      RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
    }
  }

  std::pair<std::string, cv::Rect> detectLabel(const cv::Mat &image) {
    if (image.empty()) {
      return {"", cv::Rect()};
    }

    // Ignore a small border around the image
    int margin_x = static_cast<int>(image.cols * 0.05);
    int margin_y = static_cast<int>(image.rows * 0.05);

    cv::Rect search_region(margin_x, margin_y, image.cols - 2 * margin_x,
                           image.rows - 2 * margin_y);

    cv::Mat roi = image(search_region);

    std::string detected_label = recognizeLabel(roi);

    if (detected_label.empty()) {
      return {"", cv::Rect()};
    }

    // Return the same region used for recognition as the bounding box
    return {detected_label, search_region};
  }

  std::string recognizeLabel(const cv::Mat &roi) {
    if (roi.empty()) {
      return "";
    }

    int red_pixels = 0;
    int bright_pixels = 0;
    int dark_pixels = 0;

    int total_pixels = roi.rows * roi.cols;

    for (int y = 0; y < roi.rows; ++y) {
      for (int x = 0; x < roi.cols; ++x) {

        const cv::Vec3b &pixel = roi.at<cv::Vec3b>(y, x);

        int blue = pixel[0];
        int green = pixel[1];
        int red = pixel[2];

        // Detect red pixels
        if (red > 120 && red > green + 15 && red > blue + 15) {
          red_pixels++;
        }

        // Detect bright pixels
        if (red > 180 && green > 180 && blue > 180) {
          bright_pixels++;
        }

        // Detect dark pixels
        if (red < 130 && green < 130 && blue < 130) {
          dark_pixels++;
        }
      }
    }

    double red_ratio = static_cast<double>(red_pixels) / total_pixels;
    double bright_ratio = static_cast<double>(bright_pixels) / total_pixels;
    double dark_ratio = static_cast<double>(dark_pixels) / total_pixels;

    // Preserve the previous detection logic
    if (red_ratio > 0.002) {
      return "WASTE";
    }

    bool label_visible = bright_ratio > 0.10 && dark_ratio > 0.01;

    if (label_visible) {
      return "FOOD";
    }

    return "";
  }

  std::string toUpper(const std::string &text) {
    std::string result = text;

    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    return result;
  }

  void handle_text_recognition_request(
      const std::shared_ptr<interface_pkg::srv::TextRecognition::Request>
          request,
      std::shared_ptr<interface_pkg::srv::TextRecognition::Response> response) {
    // Default response
    response->success = false;
    response->start_x = 0;
    response->start_y = 0;
    response->end_x = 0;
    response->end_y = 0;

    if (!image_received_) {
      RCLCPP_WARN(this->get_logger(), "No camera image received yet");
      return;
    }

    // Run the same detection logic used in the previous version
    auto result = detectLabel(latest_image_);
    std::string detected_label = toUpper(result.first);
    std::string requested_label = toUpper(request->label);

    RCLCPP_INFO(this->get_logger(), "Requested label: %s",
                request->label.c_str());
    RCLCPP_INFO(this->get_logger(), "Detected label: %s",
                detected_label.empty() ? "NONE" : detected_label.c_str());

    // Return success only if the requested label matches
    if (!detected_label.empty() && detected_label == requested_label) {
      response->success = true;

      const cv::Rect &bbox = result.second;

      response->start_x = bbox.x;
      response->start_y = bbox.y;
      response->end_x = bbox.x + bbox.width;
      response->end_y = bbox.y + bbox.height;
    }

    RCLCPP_INFO(this->get_logger(), "Success: %s",
                response->success ? "true" : "false");
  }

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_subscriber_;
  rclcpp::Service<interface_pkg::srv::TextRecognition>::SharedPtr service_;
  cv::Mat latest_image_;
  bool image_received_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<TextRecognitionServiceCustom>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}