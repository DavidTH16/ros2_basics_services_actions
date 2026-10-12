#include <cv_bridge/cv_bridge.h>
#include <memory>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <string>

class TextRecognitionNode : public rclcpp::Node {
public:
  TextRecognitionNode() : Node("text_recognition_node") {
    image_subscriber_ = this->create_subscription<sensor_msgs::msg::Image>(
        "/leo/camera/image_raw", 1,
        std::bind(&TextRecognitionNode::image_callback, this,
                  std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "Text Recognition Node initialized");
  }

private:

  // image callback to retrieve the sensor data
  void image_callback(const sensor_msgs::msg::Image::SharedPtr msg) {
    cv_bridge::CvImagePtr cv_ptr;

    try {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    } catch (const cv_bridge::Exception &e) {
      RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());

      return;
    }

    detectLabel(cv_ptr->image);
  }

  // method to get the region of interest
  void detectLabel(const cv::Mat &image) {
    if (image.empty()) {
      return;
    }

    /*
     * Analyze almost the whole image.
     *
     * We only remove a small border to avoid irrelevant
     * pixels near the image edges.
     *
     * This is more robust than using a fixed central ROI,
     * because the WASTE warning symbol can move up or down
     * in the image depending on how close the rover is.
     */
    int margin_x = static_cast<int>(image.cols * 0.05);
    int margin_y = static_cast<int>(image.rows * 0.05);
    cv::Rect roi_rect(margin_x, margin_y, image.cols - 2 * margin_x,
                      image.rows - 2 * margin_y);
    cv::Mat roi = image(roi_rect);
    std::string detected_label = recognizeLabel(roi);

    if (!detected_label.empty()) {
      RCLCPP_INFO(this->get_logger(), "Detected label: %s",
                  detected_label.c_str());
    }
  }

  // method to reduce the region of interest and process the text
  std::string recognizeLabel(const cv::Mat &roi) {
    /*
     * Simplified label classification.
     *
     * The WASTE box contains a reddish / pink warning
     * symbol above the text.
     *
     * The FOOD box does not contain this symbol.
     *
     * We also check that something resembling a label
     * is actually visible before returning FOOD.
     */

    int red_pixels = 0;
    int bright_pixels = 0;
    int dark_pixels = 0;

    const int total_pixels = roi.rows * roi.cols;

    for (int y = 0; y < roi.rows; ++y) {
      for (int x = 0; x < roi.cols; ++x) {

        const cv::Vec3b &pixel = roi.at<cv::Vec3b>(y, x);

        int blue = pixel[0];
        int green = pixel[1];
        int red = pixel[2];

        /*
         * Detect reddish / pink pixels from
         * the WASTE warning symbol.
         */
        if (red > 120 && red > green + 15 && red > blue + 15) {
          red_pixels++;
        }

        /*
         * Detect bright pixels from the
         * white background of the label.
         */
        if (blue > 180 && green > 180 && red > 180) {
          bright_pixels++;
        }

        /*
         * Detect dark pixels from the
         * FOOD / WASTE text.
         */
        if (blue < 130 && green < 130 && red < 130) {
          dark_pixels++;
        }
      }
    }

    double red_ratio = static_cast<double>(red_pixels) / total_pixels;
    double bright_ratio = static_cast<double>(bright_pixels) / total_pixels;
    double dark_ratio = static_cast<double>(dark_pixels) / total_pixels;

    /*
     * WASTE detection.
     *
     * Only a small percentage of the image needs
     * to contain reddish pixels because the warning
     * symbol is relatively small.
     */
    if (red_ratio > 0.002) {
      return "WASTE";
    }

    /*
     * Before returning FOOD, make sure that
     * a label is actually visible.
     *
     * We expect:
     * - enough bright pixels from the white label
     * - enough dark pixels from the text
     */
    bool label_visible = bright_ratio > 0.10 && dark_ratio > 0.01;

    if (label_visible) {
      return "FOOD";
    }

    /*
     * No label detected.
     */
    return "";
  }

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_subscriber_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<TextRecognitionNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}