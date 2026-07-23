#pragma once

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include <mutex>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "tf2_ros/transform_broadcaster.h"

#include "sentry_hardware/protocol.hpp"
#include "sentry_hardware/serial_driver.hpp"

namespace sentry_hardware
{

class ChassisInterface : public hardware_interface::SystemInterface
{
public:
    RCLCPP_SHARED_PTR_DEFINITIONS(ChassisInterface)

    hardware_interface::CallbackReturn on_init(
        const hardware_interface::HardwareInfo & info) override;

    std::vector<hardware_interface::StateInterface>
    export_state_interfaces() override;

    std::vector<hardware_interface::CommandInterface>
    export_command_interfaces() override;

    hardware_interface::CallbackReturn on_activate(
        const rclcpp_lifecycle::State & previous_state) override;

    hardware_interface::CallbackReturn on_deactivate(
        const rclcpp_lifecycle::State & previous_state) override;

    hardware_interface::return_type read(
        const rclcpp::Time & time,
        const rclcpp::Duration & period) override;

    hardware_interface::return_type write(
        const rclcpp::Time & time,
        const rclcpp::Duration & period) override;

private:
    // 底盘状态（上行帧解析结果），由 read() 填充
    double hw_vx_    = 0.0;
    double hw_vy_    = 0.0;
    double hw_yaw_   = 0.0;
    double hw_pos_x_ = 0.0;
    double hw_pos_y_ = 0.0;

    // 速度指令（下行帧数据），由 write() 读取，cmd_vel 回调写入
    double hw_cmd_vx_ = 0.0;
    double hw_cmd_vy_ = 0.0;
    double hw_cmd_wz_ = 0.0;
    std::mutex cmd_vel_mutex_;

    // 串口
    SerialDriver serial_;
    std::string  serial_port_;
    int          baudrate_ = 115200;
    bool         publish_odom_ = true;
    bool         publish_tf_ = true;
    std::string  cmd_vel_topic_ = "/cmd_vel";
    std::string  odom_topic_ = "/odom";
    std::string  odom_frame_ = "odom";
    std::string  base_frame_ = "base_footprint";

    // odom 发布 + cmd_vel 订阅节点（独立线程中 spin）
    std::shared_ptr<rclcpp::Node>                                      odom_node_;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr              odom_pub_;
    std::unique_ptr<tf2_ros::TransformBroadcaster>                     tf_broadcaster_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr         cmd_vel_sub_;
    std::thread                                                        spin_thread_;
    std::atomic<bool>                                                  spinning_{false};

    // 帧解析缓冲
    std::vector<uint8_t> rx_buf_;

    void publishOdom(const rclcpp::Time & time);
    bool parseUplink(UplinkFrame & out_frame);
};

}  // namespace sentry_hardware
