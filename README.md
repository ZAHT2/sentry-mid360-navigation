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
- [Firmware Nav2 cmd_vel patch notes](firmware_nav2_cmdvel_patch.md)
- [Known issues](ISSUES.md)

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

