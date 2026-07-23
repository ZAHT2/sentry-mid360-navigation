# Sentry MID360 Navigation Workspace

Host-side ROS 2 Humble workspace for a MID360 based sentry robot navigation stack.

The current stable workflow runs fully on the host machine, not through Docker. The stack uses MID360 lidar/IMU data for mapping, localization, real-time obstacle costmaps, and low-speed Nav2 navigation.

## Current Stable Scope

- Host-side MID360 mapping.
- Host-side low-speed navigation.
- Fixed-gimbal lidar TF.
- Point-LIO odometry and point cloud registration.
- small_gicp map relocalization.
- Nav2 planning, controller, costmaps, waypoint navigation.
- Chassis bridge from `/cmd_vel` to the verified A-board serial protocol.
- Dynamic obstacle protection: localization uses `/registered_scan`, while obstacle avoidance uses terrain costmap clouds.

## Main Runtime Chains

Mapping:

```text
MID360 -> livox_ros_driver2 -> Point-LIO -> sensor_scan_generation -> slam_toolbox -> RViz
```

Navigation:

```text
MID360 -> Point-LIO -> small_gicp -> map/odom/base_footprint
Nav2 -> /cmd_vel -> chassis_bridge_node -> /dev/ttyABoard -> A-board
```

Localization is lidar-based. Chassis odometry is published as `/chassis/odom_raw` for monitoring only and does not publish navigation TF.

## Fixed TF Used On The Real Robot

```text
base_footprint -> front_mid360:
  x=-0.001, y=+0.17185, z=0.200
  roll=0.7854, pitch=0, yaw=0

base_footprint -> gimbal_yaw:
  identity

gimbal_yaw -> gimbal_yaw_fake:
  identity
```

## Important Documents

- [MID360 host mapping SOP](mid360_宿主机建图流程.md)
- [MID360 low-speed navigation SOP](mid360_导航固定流程.md)
- [8-week internship sprint plan](实习冲刺学习计划.md)
- [Firmware Nav2 cmd_vel patch notes](firmware_nav2_cmdvel_patch.md)
- [Known issues](ISSUES.md)

## Acknowledgements

This workspace is built on top of open-source work from the RoboMaster community.

Special thanks to [@lihanchen2004](https://github.com/lihanchen2004) and the PolarBear Robotics Team for sharing the original sentry navigation stack:

- [SMBU-PolarBear-Robotics-Team/pb2025_sentry_nav](https://github.com/SMBU-PolarBear-Robotics-Team/pb2025_sentry_nav)

Our host-side mapping, TF, relocalization stability, low-speed navigation, chassis bridge, and SOP documents are based on real-robot integration and testing on top of that open-source foundation.

## Build

```bash
cd /home/zaht/sentry_nav_ws
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

## Mapping

Use the mapping SOP:

```bash
less mid360_宿主机建图流程.md
```

Daily entry point:

```bash
ros2 launch sentry_bringup mid360_mapping.launch.py use_rviz:=True use_composition:=False
```

## Navigation

Use the navigation SOP:

```bash
less mid360_导航固定流程.md
```

Daily entry point:

```bash
export MAP_ID=15
ros2 launch sentry_bringup map11_navigation.launch.py \
  use_rviz:=True \
  use_composition:=False \
  map:=/home/zaht/sentry_nav_ws/real_maps/real_map${MAP_ID}.yaml \
  prior_pcd_file:=/home/zaht/sentry_nav_ws/real_maps/real_scans${MAP_ID}.pcd \
  serial_port:=/dev/ttyABoard
```

## Map Assets

Small 2D map files under `real_maps/*.yaml`, `real_maps/*.pgm`, and `real_maps/*.png` may be kept in git as examples.

Large point cloud maps such as `real_maps/real_scans*.pcd`, rosbag recordings, zip archives, and tarballs are ignored by git. Share them through GitHub Releases or external storage when needed.

## Notes For Public Release

This workspace integrates original upstream open-source packages with local robot-specific launch files, TF, navigation parameters, and serial bridge code. Keep upstream package licenses and author metadata intact when publishing.

Do not commit:

- `build/`, `install/`, `log/`
- rosbag data
- PCD point cloud maps
- firmware zip archives
- private hardware serial numbers or personal deployment logs

---

# Sentry MID360 导航工作空间

这是一个基于 ROS 2 Humble 的宿主机端 MID360 哨兵机器人导航工作空间。

当前稳定流程已经完全运行在宿主机上，不再依赖 Docker。整套系统使用 MID360 雷达和 IMU 数据完成建图、定位、实时障碍代价地图和低速 Nav2 导航。

## 当前稳定功能

- 宿主机 MID360 建图。
- 宿主机低速导航。
- 固定云台雷达 TF。
- Point-LIO 里程计和注册点云。
- small_gicp 地图重定位。
- Nav2 路径规划、控制器、代价地图、多点导航。
- 底盘桥接节点将 `/cmd_vel` 转换为已经验证过的 A 板串口协议。
- 动态障碍保护：定位使用 `/registered_scan`，避障使用 terrain costmap 点云，避免动态障碍污染重定位。

## 主要运行链路

建图：

```text
MID360 -> livox_ros_driver2 -> Point-LIO -> sensor_scan_generation -> slam_toolbox -> RViz
```

导航：

```text
MID360 -> Point-LIO -> small_gicp -> map/odom/base_footprint
Nav2 -> /cmd_vel -> chassis_bridge_node -> /dev/ttyABoard -> A板
```

定位以雷达为准。底盘轮式里程计只发布 `/chassis/odom_raw` 做监控，不发布导航 TF，也不参与 `odom -> base_footprint`。

## 实车固定 TF

```text
base_footprint -> front_mid360:
  x=-0.001, y=+0.17185, z=0.200
  roll=0.7854, pitch=0, yaw=0

base_footprint -> gimbal_yaw:
  identity

gimbal_yaw -> gimbal_yaw_fake:
  identity
```

## 重要文档

- [MID360 宿主机建图流程](mid360_宿主机建图流程.md)
- [MID360 低速导航固定流程](mid360_导航固定流程.md)
- [8 周实习冲刺学习计划](实习冲刺学习计划.md)
- [Nav2 cmd_vel 固件补丁记录](firmware_nav2_cmdvel_patch.md)
- [已知问题](ISSUES.md)

## 致谢

本工作空间基于 RoboMaster 社区的开源项目继续整合和实车验证。

特别感谢 [@lihanchen2004](https://github.com/lihanchen2004) 和 PolarBear Robotics Team 分享原始哨兵导航栈：

- [SMBU-PolarBear-Robotics-Team/pb2025_sentry_nav](https://github.com/SMBU-PolarBear-Robotics-Team/pb2025_sentry_nav)

本仓库中的宿主机建图、TF 固化、重定位稳定化、低速导航、底盘桥接和 SOP 文档，都是在该开源基础上结合实车测试继续整理得到的。

## 构建

```bash
cd /home/zaht/sentry_nav_ws
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

## 建图

查看建图 SOP：

```bash
less mid360_宿主机建图流程.md
```

日常启动入口：

```bash
ros2 launch sentry_bringup mid360_mapping.launch.py use_rviz:=True use_composition:=False
```

## 导航

查看导航 SOP：

```bash
less mid360_导航固定流程.md
```

日常启动入口：

```bash
export MAP_ID=15
ros2 launch sentry_bringup map11_navigation.launch.py \
  use_rviz:=True \
  use_composition:=False \
  map:=/home/zaht/sentry_nav_ws/real_maps/real_map${MAP_ID}.yaml \
  prior_pcd_file:=/home/zaht/sentry_nav_ws/real_maps/real_scans${MAP_ID}.pcd \
  serial_port:=/dev/ttyABoard
```

## 地图资产

`real_maps/*.yaml`、`real_maps/*.pgm`、`real_maps/*.png` 这类较小的 2D 地图文件可以作为示例保留在 git 中。

`real_maps/real_scans*.pcd` 是大体积 3D 点云地图，rosbag、zip、tarball 等运行数据也不会进入 git。需要分享时建议使用 GitHub Releases 或外部存储。

## 公开发布注意事项

这个工作空间整合了上游开源包、本地实车 launch、TF、导航参数和串口底盘桥接代码。公开发布时需要保留上游许可证、作者信息和 package metadata。

不要提交：

- `build/`、`install/`、`log/`
- rosbag 数据
- PCD 点云地图
- 固件 zip 压缩包
- 私有硬件序列号或个人部署日志
