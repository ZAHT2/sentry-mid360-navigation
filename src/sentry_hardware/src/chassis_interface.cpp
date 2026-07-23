#include "sentry_hardware/chassis_interface.hpp"

#include <cstring>
#include <stdexcept>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "tf2/LinearMath/Quaternion.h"

namespace sentry_hardware
{

// ── 初始化：读取URDF参数，创建odom发布节点 ─────────────────────────────────

hardware_interface::CallbackReturn ChassisInterface::on_init(
    const hardware_interface::HardwareInfo & info)
{
    if (hardware_interface::SystemInterface::on_init(info) !=
        hardware_interface::CallbackReturn::SUCCESS)
    {
        return hardware_interface::CallbackReturn::ERROR;
    }

    // 从URDF的<hardware><param>标签读取串口配置
    serial_port_ = info_.hardware_parameters.count("serial_port")
        ? info_.hardware_parameters.at("serial_port")
        : "/dev/ttyUSB0";

    baudrate_ = info_.hardware_parameters.count("baudrate")
        ? std::stoi(info_.hardware_parameters.at("baudrate"))
        : 115200;

    publish_odom_ = info_.hardware_parameters.count("publish_odom")
        ? info_.hardware_parameters.at("publish_odom") == "true"
        : true;

    publish_tf_ = info_.hardware_parameters.count("publish_tf")
        ? info_.hardware_parameters.at("publish_tf") == "true"
        : true;

    cmd_vel_topic_ = info_.hardware_parameters.count("cmd_vel_topic")
        ? info_.hardware_parameters.at("cmd_vel_topic")
        : "/cmd_vel";

    odom_topic_ = info_.hardware_parameters.count("odom_topic")
        ? info_.hardware_parameters.at("odom_topic")
        : "/odom";

    odom_frame_ = info_.hardware_parameters.count("odom_frame")
        ? info_.hardware_parameters.at("odom_frame")
        : "odom";

    base_frame_ = info_.hardware_parameters.count("base_frame")
        ? info_.hardware_parameters.at("base_frame")
        : "base_footprint";

    // 独立节点：专门负责发布 /odom 和 TF
    odom_node_      = std::make_shared<rclcpp::Node>("chassis_odom_publisher");
    if (publish_odom_) {
        odom_pub_ = odom_node_->create_publisher<nav_msgs::msg::Odometry>(odom_topic_, 10);
    }
    if (publish_tf_) {
        tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(odom_node_);
    }

    RCLCPP_INFO(
        rclcpp::get_logger("sentry_hardware"),
        "ChassisInterface init: port=%s, baud=%d, cmd_vel=%s, odom=%s, publish_odom=%s, publish_tf=%s",
        serial_port_.c_str(), baudrate_, cmd_vel_topic_.c_str(), odom_topic_.c_str(),
        publish_odom_ ? "true" : "false", publish_tf_ ? "true" : "false");

    return hardware_interface::CallbackReturn::SUCCESS;
}

// ── 导出状态接口（ros2_control框架读取这些值）────────────────────────────────

std::vector<hardware_interface::StateInterface>
ChassisInterface::export_state_interfaces()
{
    std::vector<hardware_interface::StateInterface> interfaces;
    // 关节名"chassis"对应URDF <ros2_control>中定义的joint name
    interfaces.emplace_back("chassis", "vx",    &hw_vx_);
    interfaces.emplace_back("chassis", "vy",    &hw_vy_);
    interfaces.emplace_back("chassis", "yaw",   &hw_yaw_);
    interfaces.emplace_back("chassis", "pos_x", &hw_pos_x_);
    interfaces.emplace_back("chassis", "pos_y", &hw_pos_y_);
    return interfaces;
}

// ── 导出命令接口（ros2_control框架写入这些值）───────────────────────────────

std::vector<hardware_interface::CommandInterface>
ChassisInterface::export_command_interfaces()
{
    std::vector<hardware_interface::CommandInterface> interfaces;
    interfaces.emplace_back("chassis", "vx", &hw_cmd_vx_);
    interfaces.emplace_back("chassis", "vy", &hw_cmd_vy_);
    interfaces.emplace_back("chassis", "wz", &hw_cmd_wz_);
    return interfaces;
}

// ── 激活：打开串口，启动odom spin线程 ────────────────────────────────────────

hardware_interface::CallbackReturn ChassisInterface::on_activate(
    const rclcpp_lifecycle::State & /*previous_state*/)
{
    if (!serial_.open(serial_port_, baudrate_)) {
        // 无硬件模式：串口不存在时降级运行，read()/write() 已安全处理 fd<0
        RCLCPP_WARN(rclcpp::get_logger("sentry_hardware"),
            "Serial port %s not available, running without hardware", serial_port_.c_str());
    } else {
        RCLCPP_INFO(rclcpp::get_logger("sentry_hardware"),
            "Serial port opened: %s @ %d", serial_port_.c_str(), baudrate_);
    }

    // 订阅 /cmd_vel，回调在 spin_thread_ 中执行，写入时加锁
    cmd_vel_sub_ = odom_node_->create_subscription<geometry_msgs::msg::Twist>(
        cmd_vel_topic_, 10,
        [this](const geometry_msgs::msg::Twist::SharedPtr msg) {
            std::lock_guard<std::mutex> lock(cmd_vel_mutex_);
            hw_cmd_vx_ = msg->linear.x;
            hw_cmd_vy_ = msg->linear.y;
            hw_cmd_wz_ = msg->angular.z;
        });

    spinning_ = true;
    spin_thread_ = std::thread([this]() {
        rclcpp::spin(odom_node_);
    });

    return hardware_interface::CallbackReturn::SUCCESS;
}

// ── 停用：关闭串口，停止spin线程 ─────────────────────────────────────────────

hardware_interface::CallbackReturn ChassisInterface::on_deactivate(
    const rclcpp_lifecycle::State & /*previous_state*/)
{
    serial_.close();
    spinning_ = false;
    rclcpp::shutdown();
    if (spin_thread_.joinable()) {
        spin_thread_.join();
    }
    return hardware_interface::CallbackReturn::SUCCESS;
}

// ── read()：读串口 → 解析上行帧 → 更新状态接口 → 发布odom ───────────────────

hardware_interface::return_type ChassisInterface::read(
    const rclcpp::Time & time,
    const rclcpp::Duration & /*period*/)
{
    UplinkFrame frame;
    if (parseUplink(frame)) {
        hw_vx_    = static_cast<double>(frame.vx);
        hw_vy_    = static_cast<double>(frame.vy);
        hw_yaw_   = static_cast<double>(frame.yaw);
        hw_pos_x_ = static_cast<double>(frame.pos_x);
        hw_pos_y_ = static_cast<double>(frame.pos_y);
        publishOdom(time);
    }
    return hardware_interface::return_type::OK;
}

// ── write()：读命令接口 → 打包下行帧 → 写串口 ───────────────────────────────

hardware_interface::return_type ChassisInterface::write(
    const rclcpp::Time & /*time*/,
    const rclcpp::Duration & /*period*/)
{
    double vx, vy, wz;
    {
        std::lock_guard<std::mutex> lock(cmd_vel_mutex_);
        vx = hw_cmd_vx_;
        vy = hw_cmd_vy_;
        wz = hw_cmd_wz_;
    }

    DownlinkFrame frame;
    frame.vx = static_cast<float>(vx);
    frame.vy = static_cast<float>(vy);
    frame.wz = static_cast<float>(wz);
    crc16_fill(frame);

    serial_.write(reinterpret_cast<const uint8_t *>(&frame), sizeof(frame));
    return hardware_interface::return_type::OK;
}

// ── 发布odom和TF ─────────────────────────────────────────────────────────────

void ChassisInterface::publishOdom(const rclcpp::Time & time)
{
    if (!publish_odom_ && !publish_tf_) {
        return;
    }

    // 航向角 → 四元数
    tf2::Quaternion q;
    q.setRPY(0.0, 0.0, hw_yaw_);

    // nav_msgs/Odometry
    nav_msgs::msg::Odometry odom;
    odom.header.stamp    = time;
    odom.header.frame_id = odom_frame_;
    odom.child_frame_id  = base_frame_;

    odom.pose.pose.position.x  = hw_pos_x_;
    odom.pose.pose.position.y  = hw_pos_y_;
    odom.pose.pose.position.z  = 0.0;
    odom.pose.pose.orientation.x = q.x();
    odom.pose.pose.orientation.y = q.y();
    odom.pose.pose.orientation.z = q.z();
    odom.pose.pose.orientation.w = q.w();

    odom.twist.twist.linear.x  = hw_vx_;
    odom.twist.twist.linear.y  = hw_vy_;
    odom.twist.twist.angular.z = hw_cmd_wz_;  // C板未上报wz时用指令值近似

    if (publish_odom_ && odom_pub_) {
        odom_pub_->publish(odom);
    }

    // TF: odom → base_footprint
    if (!publish_tf_ || !tf_broadcaster_) {
        return;
    }

    geometry_msgs::msg::TransformStamped tf_msg;
    tf_msg.header.stamp    = time;
    tf_msg.header.frame_id = odom_frame_;
    tf_msg.child_frame_id  = base_frame_;
    tf_msg.transform.translation.x = hw_pos_x_;
    tf_msg.transform.translation.y = hw_pos_y_;
    tf_msg.transform.translation.z = 0.0;
    tf_msg.transform.rotation      = odom.pose.pose.orientation;

    tf_broadcaster_->sendTransform(tf_msg);
}

// ── 帧解析状态机：在rx_buf_中搜索合法的上行帧 ───────────────────────────────

bool ChassisInterface::parseUplink(UplinkFrame & out_frame)
{
    uint8_t tmp[128];
    int n = serial_.read(tmp, sizeof(tmp), 5);
    if (n <= 0) return false;

    rx_buf_.insert(rx_buf_.end(), tmp, tmp + n);

    constexpr size_t FRAME_LEN = sizeof(UplinkFrame);

    while (rx_buf_.size() >= FRAME_LEN) {
        // 找帧头
        if (rx_buf_[0] != FRAME_SOF || rx_buf_[1] != CMD_UPLINK) {
            rx_buf_.erase(rx_buf_.begin());
            continue;
        }
        // CRC校验
        if (!crc16_verify(rx_buf_.data(), FRAME_LEN)) {
            rx_buf_.erase(rx_buf_.begin());
            continue;
        }
        // 解出完整帧
        memcpy(&out_frame, rx_buf_.data(), FRAME_LEN);
        rx_buf_.erase(rx_buf_.begin(), rx_buf_.begin() + FRAME_LEN);
        return true;
    }

    // 防止缓冲区无限增长（正常情况不会超过几帧大小）
    if (rx_buf_.size() > FRAME_LEN * 4) {
        rx_buf_.erase(rx_buf_.begin(), rx_buf_.end() - FRAME_LEN);
    }

    return false;
}

}  // namespace sentry_hardware

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(
    sentry_hardware::ChassisInterface,
    hardware_interface::SystemInterface)
