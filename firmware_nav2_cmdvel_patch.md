# Nav2 接入实际底盘固件检查结论

## 当前实际链路

MiniPC / Docker Nav2 输出 `/cmd_vel`。

本地 `chassis_bridge_node` 将 `/cmd_vel` 打包成 16 字节 USB CDC 帧：

```text
0xA5 0x02 float vx float vy float wz uint16 crc16
```

GIMBAL A 板 `ExternalTask` 中的 `VisionNavigation` 能正确接收这个帧，并保存为：

```cpp
cmd_vx_ = frame.vx;
cmd_vy_ = frame.vy;
cmd_wz_ = frame.wz;
```

GIMBAL A 板 `CommunicationTask` 再把它塞入 A 板到 C 板的 64 字节板间通信帧：

```cpp
navigation.Nav_X  = Communication::navigation_for_MinPC.getCmdVx();
navigation.Nav_Y  = Communication::navigation_for_MinPC.getCmdVy();
navigation.Nav_Wz = Communication::navigation_for_MinPC.getCmdWz();
```

CHASSIS C 板 `Communication` 能从 UART1 收到并解析：

```cpp
float getNavigation_x();
float getNavigation_y();
float getNavigation_wz();
```

所以：上位机协议、A 板接收、A->C 转发结构整体是对得上的。

## 当前问题

C 板 `ChassisTask::AutoHandler::AutoTarget()` 当前没有把 `getNavigation_x/y/wz()` 当作速度执行。

它进入 `AutoState` 后运行的是固化点导航逻辑：

```text
MOVE_TO_A / WAIT_AT_A / TURN_TO_B / MOVE_TO_B / WAIT_AT_B
```

目标点来自 `VariableConfig.hpp`：

```cpp
CHASSIS_TASK_POINT_NAV_POINT_A_Y 2.2f
CHASSIS_TASK_POINT_NAV_POINT_B_Y -2.2f
...
```

这会导致 Nav2 发 `/cmd_vel` 后，C 板并不会直接执行 Nav2 的速度。

## 遥控模式门控

GIMBAL A 板只有在遥控器：

```text
左拨杆 MIDDLE，右拨杆 MIDDLE
```

时才会下发 `Auto_mode = 1`。

C 板状态优先级是：

```text
Stop > Auto > Universal > Follow > Rotating
```

所以测试 Nav2 实车执行时，需要避免 stop，并让 A 板进入 Auto。

## 推荐第一阶段固件改法

先不要用 C 板固化点导航，让 C 板 Auto 模式直接执行 Nav2 下发的速度。

在 CHASSIS 工程的 `User/APP_Task/ChassisTask.cpp` 中，把 `AutoHandler::AutoTarget()` 改成直接速度模式。

注意坐标系：

```text
ROS/Nav2: x 向前，y 向左，wz 逆时针为正
C 板注释: x 向右，y 向前
```

因此平移建议先用：

```cpp
const float ros_vx = Gimbal_to_Chassis_Data.getNavigation_x();
const float ros_vy = Gimbal_to_Chassis_Data.getNavigation_y();
const float ros_wz = Gimbal_to_Chassis_Data.getNavigation_wz();

const float target_vx = ClampFloat(-ros_vy * Navigation_Pram, -kPointNavMaxCmd, kPointNavMaxCmd);
const float target_vy = ClampFloat( ros_vx * Navigation_Pram, -kPointNavMaxCmd, kPointNavMaxCmd);
const float target_vw = ClampFloat( ros_wz * Navigation_Pram, -kPointNavTurnMaxCmd, kPointNavTurnMaxCmd);

tar_vx.Calc(target_vx);
tar_vy.Calc(target_vy);
tar_vw.Calc(target_vw);

Chassis_Data.vx = tar_vx.x1;
Chassis_Data.vy = tar_vy.x1;
Chassis_Data.vw = tar_vw.x1;
```

如果实测 `/cmd_vel angular.z > 0` 时车不是逆时针转，就把 `target_vw` 前面加负号。

## 实车验证顺序

1. 底盘架空。
2. 遥控器切到 Auto：左中、右中。
3. 上位机发布小速度：

```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
"{linear: {x: 0.2, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}"
```

期望：车轮趋势为车头前进。

4. 测横移：

```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
"{linear: {x: 0.0, y: 0.2, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}"
```

期望：车轮趋势为车体向左。

5. 测旋转：

```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
"{linear: {x: 0.0, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.5}}"
```

期望：俯视逆时针旋转。

