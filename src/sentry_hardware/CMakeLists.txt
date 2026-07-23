cmake_minimum_required(VERSION 3.8)
project(sentry_hardware)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2 REQUIRED)

# ── 硬件接口动态库（ros2_control插件）────────────────────────────────────────
add_library(sentry_hardware SHARED
  src/protocol.cpp
  src/serial_driver.cpp
  src/chassis_interface.cpp
)

target_include_directories(sentry_hardware PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(sentry_hardware
  rclcpp
  hardware_interface
  pluginlib
  rclcpp_lifecycle
  nav_msgs
  geometry_msgs
  tf2_ros
  tf2
)

set_target_properties(sentry_hardware PROPERTIES
  INSTALL_RPATH "/opt/ros/humble/lib"
  INSTALL_RPATH_USE_LINK_PATH FALSE
)

add_executable(chassis_bridge_node
  src/protocol.cpp
  src/serial_driver.cpp
  src/chassis_bridge_node.cpp
)

target_include_directories(chassis_bridge_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(chassis_bridge_node
  rclcpp
  nav_msgs
  geometry_msgs
  tf2_ros
  tf2
)

set_target_properties(chassis_bridge_node PROPERTIES
  INSTALL_RPATH "/opt/ros/humble/lib"
  INSTALL_RPATH_USE_LINK_PATH FALSE
)

# pluginlib 需要可见性宏
target_compile_definitions(sentry_hardware PRIVATE "SENTRY_HARDWARE_BUILDING_DLL")

pluginlib_export_plugin_description_file(hardware_interface sentry_hardware.xml)

# ── 安装 ─────────────────────────────────────────────────────────────────────
install(TARGETS sentry_hardware
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS chassis_bridge_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(FILES sentry_hardware.xml
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_include_directories(include)
ament_export_libraries(sentry_hardware)
ament_export_dependencies(
  rclcpp
  hardware_interface
  pluginlib
  rclcpp_lifecycle
  nav_msgs
  geometry_msgs
  tf2_ros
  tf2
)

ament_package()
