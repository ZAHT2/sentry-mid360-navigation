# 未解决事项

## [待确认] Mid360 网络 IP 配置

**描述：** 接硬件前需确认 Mid360 实际 IP 地址并修改配置文件。

**操作步骤：**
1. 查看雷达标签上的序列号，末位决定 IP（默认 `192.168.1.1x`）
2. 修改 `src/sentry_bringup/config/mid360_config.json`：
   - `"ip"` 字段改为雷达实际 IP
   - `"cmd_data_ip"` 等 host 字段改为 MiniPC 网卡 IP
3. 配置 MiniPC 网卡为同网段静态 IP（例如 `192.168.1.50`）：
   ```bash
   sudo ip addr add 192.168.1.50/24 dev <网卡名>
   # 查网卡名：ip addr
   ```
4. 验证连通性：`ping 192.168.1.1x`

**相关文件：** `src/sentry_bringup/config/mid360_config.json`

---

## [待确认] ros2_control RT 调度权限

**描述：** 启动时出现警告，不影响功能但影响实时性。

**报错：**
```
Could not enable FIFO RT scheduling policy: Operation not permitted
```

**解决方法：** 给 MiniPC 上的用户添加实时调度权限：
```bash
sudo bash -c 'echo "@realtime - rtprio 99" >> /etc/security/limits.conf'
sudo bash -c 'echo "@realtime - memlock unlimited" >> /etc/security/limits.conf'
sudo groupadd realtime
sudo usermod -aG realtime $USER
# 重新登录后生效
```

**相关文件：** `src/sentry_bringup/launch/sentry.launch.py`

---

## [已知] conda 与 ROS2 Python 版本冲突

**描述：** conda 默认激活时使用 Python 3.12，ROS2 Humble 需要 Python 3.10。运行任何 ROS2 工具（ros2 run、ros2 launch、colcon build）前必须先退出 conda。

**错误表现：**
```
ModuleNotFoundError: No module named 'rclpy._rclpy_pybind11'
```

**解决方案：** 每次开新终端运行 ROS2 前执行：
```bash
conda deactivate
source /opt/ros/humble/setup.bash
```

**根治方案（可选）：** 在 `~/.bashrc` 末尾添加：
```bash
# 防止 conda base 自动激活干扰 ROS2
conda config --set auto_activate_base false
```
