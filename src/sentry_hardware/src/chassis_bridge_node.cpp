#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "geometry_msgs/msg/transform_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_ros/transform_broadcaster.h"

#include "sentry_hardware/protocol.hpp"
#include "sentry_hardware/serial_driver.hpp"

namespace sentry_hardware
{

class ChassisBridgeNode : public rclcpp::Node
{
public:
    ChassisBridgeNode()
    : Node("chassis_bridge_node")
    {
        serial_port_ = declare_parameter<std::string>("serial_port", "/dev/ttyABoard");
        baudrate_ = declare_parameter<int>("baudrate", 115200);
        cmd_vel_topic_ = declare_parameter<std::string>("cmd_vel_topic", "/cmd_vel");
        odom_topic_ = declare_parameter<std::string>("odom_topic", "/chassis/odom_raw");
        odom_frame_ = declare_parameter<std::string>("odom_frame", "chassis_odom");
        base_frame_ = declare_parameter<std::string>("base_frame", "base_footprint");
        publish_odom_ = declare_parameter<bool>("publish_odom", true);
        publish_tf_ = declare_parameter<bool>("publish_tf", false);
        write_rate_hz_ = declare_parameter<double>("write_rate_hz", 50.0);
        read_timeout_ms_ = declare_parameter<int>("read_timeout_ms", 2);
        cmd_timeout_sec_ = declare_parameter<double>("cmd_timeout_sec", 0.5);
        cmd_vx_from_ros_x_ = declare_parameter<double>("cmd_vx_from_ros_x", 1.0);
        cmd_vx_from_ros_y_ = declare_parameter<double>("cmd_vx_from_ros_y", 0.0);
        cmd_vy_from_ros_x_ = declare_parameter<double>("cmd_vy_from_ros_x", 0.0);
        cmd_vy_from_ros_y_ = declare_parameter<double>("cmd_vy_from_ros_y", 1.0);
        cmd_wz_scale_ = declare_parameter<double>("cmd_wz_scale", 1.0);
        last_cmd_time_ = now();

        if (publish_odom_) {
            odom_pub_ = create_publisher<nav_msgs::msg::Odometry>(odom_topic_, 10);
        }
        if (publish_tf_) {
            tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
        }

        cmd_vel_sub_ = create_subscription<geometry_msgs::msg::Twist>(
            cmd_vel_topic_, 10,
            [this](const geometry_msgs::msg::Twist::SharedPtr msg) {
                std::lock_guard<std::mutex> lock(cmd_mutex_);
                cmd_vx_ = msg->linear.x;
                cmd_vy_ = msg->linear.y;
                cmd_wz_ = msg->angular.z;
                last_cmd_time_ = now();
                if (std::fabs(cmd_vx_) > 1e-4 || std::fabs(cmd_vy_) > 1e-4 || std::fabs(cmd_wz_) > 1e-4) {
                    RCLCPP_INFO_THROTTLE(
                        get_logger(), *get_clock(), 1000,
                        "Received /cmd_vel: vx=%.3f vy=%.3f wz=%.3f",
                        cmd_vx_, cmd_vy_, cmd_wz_);
                }
            });

        if (!serial_.open(serial_port_, baudrate_)) {
            RCLCPP_ERROR(
                get_logger(),
                "Failed to open serial port %s @ %d. /cmd_vel will not reach chassis.",
                serial_port_.c_str(), baudrate_);
        } else {
            RCLCPP_INFO(
                get_logger(),
                "Opened serial port %s @ %d, subscribing %s, publishing odom=%s, tf=%s",
                serial_port_.c_str(), baudrate_, cmd_vel_topic_.c_str(),
                publish_odom_ ? odom_topic_.c_str() : "false",
                publish_tf_ ? "true" : "false");
        }
        RCLCPP_INFO(
            get_logger(),
            "Command mapping: chassis_vx=%.3f*ros_x + %.3f*ros_y, chassis_vy=%.3f*ros_x + %.3f*ros_y, chassis_wz=%.3f*ros_wz",
            cmd_vx_from_ros_x_, cmd_vx_from_ros_y_, cmd_vy_from_ros_x_, cmd_vy_from_ros_y_,
            cmd_wz_scale_);

        const auto period = std::chrono::duration<double>(1.0 / write_rate_hz_);
        io_timer_ = create_wall_timer(
            std::chrono::duration_cast<std::chrono::nanoseconds>(period),
            [this]() {
                readSerial();
                writeCommand();
            });
    }

private:
    void readSerial()
    {
        UplinkFrame frame;
        if (!parseUplink(frame)) {
            return;
        }

        hw_vx_ = frame.vx;
        hw_vy_ = frame.vy;
        hw_yaw_ = frame.yaw;
        hw_pos_x_ = frame.pos_x;
        hw_pos_y_ = frame.pos_y;

        if (std::fabs(hw_vx_) > 1e-4 || std::fabs(hw_vy_) > 1e-4) {
            RCLCPP_INFO_THROTTLE(
                get_logger(), *get_clock(), 1000,
                "Received chassis feedback: vx=%.3f vy=%.3f yaw=%.3f pos=(%.3f, %.3f)",
                hw_vx_, hw_vy_, hw_yaw_, hw_pos_x_, hw_pos_y_);
        }

        publishOdom(now());
    }

    void writeCommand()
    {
        double vx = 0.0;
        double vy = 0.0;
        double wz = 0.0;
        {
            std::lock_guard<std::mutex> lock(cmd_mutex_);
            vx = cmd_vx_;
            vy = cmd_vy_;
            wz = cmd_wz_;

            if (cmd_timeout_sec_ > 0.0 && (now() - last_cmd_time_).seconds() > cmd_timeout_sec_) {
                if (std::fabs(cmd_vx_) > 1e-4 || std::fabs(cmd_vy_) > 1e-4 || std::fabs(cmd_wz_) > 1e-4) {
                    RCLCPP_WARN_THROTTLE(
                        get_logger(), *get_clock(), 1000,
                        "No /cmd_vel for %.2fs; sending zero velocity",
                        cmd_timeout_sec_);
                }
                vx = 0.0;
                vy = 0.0;
                wz = 0.0;
                cmd_vx_ = 0.0;
                cmd_vy_ = 0.0;
                cmd_wz_ = 0.0;
            }
        }

        const double chassis_vx = cmd_vx_from_ros_x_ * vx + cmd_vx_from_ros_y_ * vy;
        const double chassis_vy = cmd_vy_from_ros_x_ * vx + cmd_vy_from_ros_y_ * vy;
        const double chassis_wz = cmd_wz_scale_ * wz;

        DownlinkFrame frame;
        frame.vx = static_cast<float>(chassis_vx);
        frame.vy = static_cast<float>(chassis_vy);
        frame.wz = static_cast<float>(chassis_wz);
        crc16_fill(frame);

        const int written = serial_.write(reinterpret_cast<const uint8_t *>(&frame), sizeof(frame));
        if (std::fabs(vx) > 1e-4 || std::fabs(vy) > 1e-4 || std::fabs(wz) > 1e-4) {
            RCLCPP_INFO_THROTTLE(
                get_logger(), *get_clock(), 1000,
                "Wrote chassis cmd: ros=(%.3f, %.3f, %.3f) chassis=(%.3f, %.3f, %.3f) bytes=%d",
                vx, vy, wz, chassis_vx, chassis_vy, chassis_wz, written);
        }
        if (written < 0 && serial_.isOpen()) {
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "Serial write failed");
        }
    }

    bool parseUplink(UplinkFrame & out_frame)
    {
        uint8_t tmp[128];
        const int n = serial_.read(tmp, sizeof(tmp), read_timeout_ms_);
        if (n <= 0) {
            return false;
        }

        rx_buf_.insert(rx_buf_.end(), tmp, tmp + n);

        constexpr size_t frame_len = sizeof(UplinkFrame);
        while (rx_buf_.size() >= frame_len) {
            if (rx_buf_[0] != FRAME_SOF || rx_buf_[1] != CMD_UPLINK) {
                rx_buf_.erase(rx_buf_.begin());
                continue;
            }
            if (!crc16_verify(rx_buf_.data(), frame_len)) {
                rx_buf_.erase(rx_buf_.begin());
                continue;
            }

            std::memcpy(&out_frame, rx_buf_.data(), frame_len);
            rx_buf_.erase(rx_buf_.begin(), rx_buf_.begin() + frame_len);
            return true;
        }

        if (rx_buf_.size() > frame_len * 4) {
            rx_buf_.erase(rx_buf_.begin(), rx_buf_.end() - frame_len);
        }
        return false;
    }

    void publishOdom(const rclcpp::Time & stamp)
    {
        if (!publish_odom_ && !publish_tf_) {
            return;
        }

        tf2::Quaternion q;
        q.setRPY(0.0, 0.0, hw_yaw_);

        nav_msgs::msg::Odometry odom;
        odom.header.stamp = stamp;
        odom.header.frame_id = odom_frame_;
        odom.child_frame_id = base_frame_;
        odom.pose.pose.position.x = hw_pos_x_;
        odom.pose.pose.position.y = hw_pos_y_;
        odom.pose.pose.position.z = 0.0;
        odom.pose.pose.orientation.x = q.x();
        odom.pose.pose.orientation.y = q.y();
        odom.pose.pose.orientation.z = q.z();
        odom.pose.pose.orientation.w = q.w();
        odom.twist.twist.linear.x = hw_vx_;
        odom.twist.twist.linear.y = hw_vy_;
        odom.twist.twist.angular.z = cmd_wz_;

        if (publish_odom_ && odom_pub_) {
            odom_pub_->publish(odom);
        }

        if (!publish_tf_ || !tf_broadcaster_) {
            return;
        }

        geometry_msgs::msg::TransformStamped tf_msg;
        tf_msg.header.stamp = stamp;
        tf_msg.header.frame_id = odom_frame_;
        tf_msg.child_frame_id = base_frame_;
        tf_msg.transform.translation.x = hw_pos_x_;
        tf_msg.transform.translation.y = hw_pos_y_;
        tf_msg.transform.translation.z = 0.0;
        tf_msg.transform.rotation = odom.pose.pose.orientation;
        tf_broadcaster_->sendTransform(tf_msg);
    }

    SerialDriver serial_;
    std::string serial_port_;
    int baudrate_ = 115200;
    std::string cmd_vel_topic_;
    std::string odom_topic_;
    std::string odom_frame_;
    std::string base_frame_;
    bool publish_odom_ = true;
    bool publish_tf_ = false;
    double write_rate_hz_ = 50.0;
    int read_timeout_ms_ = 2;
    double cmd_timeout_sec_ = 0.5;
    double cmd_vx_from_ros_x_ = 1.0;
    double cmd_vx_from_ros_y_ = 0.0;
    double cmd_vy_from_ros_x_ = 0.0;
    double cmd_vy_from_ros_y_ = 1.0;
    double cmd_wz_scale_ = 1.0;

    double cmd_vx_ = 0.0;
    double cmd_vy_ = 0.0;
    double cmd_wz_ = 0.0;
    rclcpp::Time last_cmd_time_;
    double hw_vx_ = 0.0;
    double hw_vy_ = 0.0;
    double hw_yaw_ = 0.0;
    double hw_pos_x_ = 0.0;
    double hw_pos_y_ = 0.0;

    std::mutex cmd_mutex_;
    std::vector<uint8_t> rx_buf_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
    rclcpp::TimerBase::SharedPtr io_timer_;
};

}  // namespace sentry_hardware

int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<sentry_hardware::ChassisBridgeNode>());
    rclcpp::shutdown();
    return 0;
}
