# Sentry MID360 导航调试交接

## 1. 我们在做什么

目标是在 ROS 2 Humble 下跑通 `sentry-mid360-navigation` 实车功能包，完成以下链路：

- Livox MID360 点云和 IMU输入
- Point-LIO 里程计
- `small_gicp_relocalization` 的 `map -> odom` 定位
- Nav2 地图、规划、控制、路点导航
- `/dev/ttyACM0` 底盘串口通信和速度控制

主要启动命令：

```bash
export FASTDDS_BUILTIN_TRANSPORTS=UDPv4
export RMW_FASTRTPS_USE_SHM=0
source /opt/ros/humble/setup.bash
source install/setup.bash

ros2 launch sentry_bringup map11_navigation.launch.py \
  use_rviz:=True \
  use_composition:=False \
  map:=$PWD/maps/zd.yaml \
  prior_pcd_file:=$PWD/maps/scans.pcd \
  serial_port:=/dev/ttyACM0
```

## 2. 已完成的工作

### 编译和硬件通信

- 全工作区曾成功编译：`19 packages finished`。
- 底盘真实设备确认是 `/dev/ttyACM0`，不是 `/dev/ttyABoard`。
- 底盘桥接实测可打开 `115200` 串口。
- `/chassis/odom_raw` 实测约 `50 Hz`。
- `/livox/lidar` 实测约 `20 Hz`。
- `/cloud_registered` 实测约 `20 Hz`。
- Livox 配置已包含雷达 IP `192.168.1.104`，主机接收 IP 为 `192.168.1.50`。

### TF 和启动文件

- `map11_navigation.launch.py` 中存在 `base_footprint -> base_link` 单位 TF。
- 雷达 TF 为 `base_footprint -> front_mid360`：
  - x = `-0.001`
  - y = `0.17185`
  - z = `0.200`
  - roll = `0.7854 rad`（45 度）
  - pitch/yaw = `0`
- `map -> gimbal_yaw_fake` 曾能正常查询。

### 地图和点云文件

- 栅格地图：`maps/zd.yaml`、`maps/zd.pgm`。
- 先验点云：`maps/scans.pcd`，约 157 MB。
- `maps/zd.pcd` 不存在，不能作为 `prior_pcd_file`。

### Nav2 生命周期

- 曾通过 lifecycle manager 的单次 startup 将以下节点全部恢复到 `active [3]`：
  - `controller_server`
  - `smoother_server`
  - `planner_server`
  - `behavior_server`
  - `bt_navigator`
  - `waypoint_follower`
  - `velocity_smoother`
- 恢复命令（只在节点处于 unconfigured/inactive 且没有另一个 transition 正在运行时使用）：

```bash
ros2 service call /lifecycle_manager_navigation/manage_nodes \
  nav2_msgs/srv/ManageLifecycleNodes "{command: 0}"
```

### 已观察到的导航行为

- “走约 4 秒后停车”的一次案例不是控制器报错，而是动作状态 `4`（`SUCCEEDED`），Nav2 认为路点已经到达后主动输出零速度。
- “路点发出但没有路径”的明确日志是：

```text
Either of the start or goal pose are an obstacle!
```

- 失败路点曾包括 `(-0.22, -4.82)` 和 `(0.13, 0.62)`。
- 当时机器人定位约 `(0.08, 0.39)`，后一个路点离机器人约 0.24 m，小于当前 `robot_radius: 0.32`，不适合作为首个路点。

## 3. 当前磁盘状态

工作区不是干净状态，以下文件有未提交修改，禁止无差别恢复：

```text
mid360_导航固定流程.md
src/pb2025_nav_bringup/config/reality/nav2_params.yaml
src/sentry_bringup/launch/map11_navigation.launch.py
src/sentry_navigation/CMakeLists.txt
src/small_gicp_relocalization/include/small_gicp_relocalization/small_gicp_relocalization.hpp
src/small_gicp_relocalization/launch/small_gicp_relocalization_launch.py
src/small_gicp_relocalization/src/small_gicp_relocalization.cpp
maps/zd.pgm（未跟踪）
maps/zd.yaml（未跟踪）
```

特别注意：速度参数在会话结束前又被外部修改。当前磁盘实际值是：

```yaml
v_linear_min: -2.5
v_linear_max: 2.5
max_velocity: [2.5, 2.5, 3.0]
min_velocity: [-2.5, -2.5, -3.0]
max_accel: [4.5, 4.5, 5.0]
max_decel: [-4.5, -4.5, -5.0]
```

这与聊天中最后要求的“最高 3、最低 1.2”不一致。继续工作前必须先向用户确认最终期望，再修改控制器和平滑器两处；不要仅改一处。

## 4. 当前卡点

### 首要问题：定位持续失配

`small_gicp_relocalization` 长期出现：

```text
Keeping previous map->odom transform after rejected GICP update. reason=error_too_high
```

观察过的 error 常在 20-50，严重时超过 200；同时存在约 0.4-0.46 m 的 translation jump。节点因此保留旧的 `map -> odom`。直接后果：

- RViz 白色实时点云与先验点云/地图错位。
- Nav2 可能把机器人起点判为障碍。
- Nav2 可能错误地认为所有路点已经到达。
- 路点规划失败或车辆提前停止。

当前配置中 GICP 初始位姿为全零：

```yaml
small_gicp_relocalization:
  ros__parameters:
    init_pose: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
```

需要根据实车在 `scans.pcd` 中的真实位置设置正确初始位姿，或确认 RViz `2D Pose Estimate` 是否确实被该节点消费并更新 `map -> odom`。

### 次要问题：生命周期启动偶发卡住

- 导航 lifecycle manager 在启动节点 15 秒后才启动。
- 曾出现 controller/planner 为 inactive、BT 为 unconfigured。
- 曾卡在 `behavior_server` configure 阶段。
- 用户经常在 15 秒左右按 Ctrl+C，导致节点尚未 active 就退出。
- 必须启动一次后等待至少 20-30 秒，再检查状态。

### 代价地图较保守

当前实车配置：

```yaml
robot_radius: 0.32
inflation_radius: 1.0
```

窄通道和靠墙路点可规划空间较小。但不要在定位错位时先缩小半径、关闭障碍层或降低安全距离，这会掩盖根因。

## 5. 下一步计划

1. 启动时固定使用 `maps/scans.pcd` 和 `/dev/ttyACM0`，等待 30 秒，不要中断。
2. 确认所有生命周期节点都是 `active [3]`：

```bash
ros2 lifecycle get /map_server
ros2 lifecycle get /controller_server
ros2 lifecycle get /planner_server
ros2 lifecycle get /bt_navigator
```

3. 检查输入和 TF：

```bash
timeout 5 ros2 topic hz /livox/lidar
timeout 5 ros2 topic hz /cloud_registered
timeout 5 ros2 topic hz /chassis/odom_raw
timeout 4 ros2 run tf2_ros tf2_echo base_footprint front_mid360
timeout 4 ros2 run tf2_ros tf2_echo map gimbal_yaw_fake
```

4. 在 RViz 中对齐实时点云与 `scans.pcd`，记录正确的 `[x, y, z, roll, pitch, yaw]`，写入 GICP `init_pose`。
5. 只有在 GICP 不再连续 `error_too_high` 后再测试导航。
6. 第一个路点放在机器人 0.5 m 之外、静态地图空白且远离膨胀区的位置。
7. 同时监控动作结果：

```bash
ros2 topic echo /navigate_through_poses/_action/status
```

状态 `4` 是成功，不是故障；如果实车未到但状态为 4，继续修定位，不要调控制器速度。
8. 定位稳定后再评估 `robot_radius`、`inflation_radius` 和速度参数。

## 6. 绝对不要再踩的坑

- 不要使用不存在的 `maps/zd.pcd`。它曾导致 GICP `exit code -11`。
- 不要使用 `/dev/ttyABoard`。当前实车串口是 `/dev/ttyACM0`。
- 不要在 launch 启动后 15-20 秒内按 Ctrl+C；这时生命周期管理器可能刚开始工作。
- 不要连续发送 lifecycle `reset` 和 `startup`。一次 reset 曾在后台延迟执行，在导航开始后又把全栈反激活，导致车辆走几秒后停车；随后重复 configure 触发：

```text
parameter 'intensity_voxel_layer.z_voxels' has already been declared
```

- 不要逐个手动激活 Nav2 节点。应该让 lifecycle manager 按顺序管理完整节点链。
- 不要看到 `/map` 有 publisher 就认为导航已就绪。必须检查 controller/planner/BT 都是 active。
- 不要只看 `/cmd_vel`。当前启动链存在 remap，底盘实际链路涉及 `/cmd_vel_controller` 和 `/cmd_vel_nav2_result`。
- 不要把 `v_linear_min` 写成正数。它是反向速度下限，应使用负值，例如 `-1.2`。
- 不要只提高 controller 的最大速度；`velocity_smoother.max_velocity` 也必须同步，否则仍会被限速。
- 不要把 `min_approach_linear_velocity` 设为 1.2 或 2.0 来表达“最低速度”，否则机器人可能高速冲过终点。
- 不要在 GICP 持续拒绝时通过关闭障碍层、缩小机器人半径来强行出路径。
- 不要误判 RViz GLSL 错误。`active samplers with a different type...` 通常只是 OpenGL 地图纹理显示问题，不是 Nav2 规划失败原因。
- Livox 启动最初出现 `Storage point data failed` 后若随后成功进入 Normal、启用 IMU 且话题保持 20 Hz，通常是设备索引注册前的瞬时日志；先测频率，不要立即改 IP。
- 工作区有用户未提交修改。禁止 `git reset --hard`、整仓 `git restore` 或覆盖未知改动。

## 7. 快速判障表

| 现象 | 优先检查 |
| --- | --- |
| 无规划路径 | planner 日志是否为 start/goal obstacle；GICP 是否失配 |
| 车完全不动 | controller/planner/BT 是否 active；速度链是否有订阅者 |
| 走几秒停车 | action status 是否为 4；是否错误判定到达；是否有 lifecycle reset |
| 点云不显示 | `/livox/lidar`、`/cloud_registered` 频率和 TF |
| 白色点云偏移 | 雷达 TF、GICP init_pose、`map -> odom` |
| `Node not found` | launch 是否仍运行、DDS 环境变量、是否等待足够时间 |
| GICP 段错误 | `prior_pcd_file` 是否存在且是有效 PCD |
