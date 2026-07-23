#!/usr/bin/env python3
# 精简键盘遥控：发布 geometry_msgs/Twist 到指定话题（默认 /red_standard_robot1/cmd_vel）
# 用法（容器内）: python3 /root/key_teleop.py [cmd_vel话题]
# 按键: w/s 前后, a/d 左右平移, q/e 左右转, 空格急停, Ctrl-C 退出
import sys, termios, tty, select
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist

HELP = """
============ 键盘遥控建图 ============
  w / s : 前进 / 后退
  a / d : 左移 / 右移
  q / e : 左转 / 右转
  空格  : 停止
  +/-   : 调整速度
  Ctrl-C: 退出
=====================================
"""

MOVE = {'w': (1, 0, 0), 's': (-1, 0, 0), 'a': (0, 1, 0),
        'd': (0, -1, 0), 'q': (0, 0, 1), 'e': (0, 0, -1)}


def get_key(settings):
    tty.setraw(sys.stdin.fileno())
    r, _, _ = select.select([sys.stdin], [], [], 0.1)
    key = sys.stdin.read(1) if r else ''
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, settings)
    return key


def main():
    topic = sys.argv[1] if len(sys.argv) > 1 else '/red_standard_robot1/cmd_vel'
    rclpy.init()
    node = Node('key_teleop')
    pub = node.create_publisher(Twist, topic, 10)
    settings = termios.tcgetattr(sys.stdin)
    speed = 0.5
    print(HELP)
    print(f'发布到: {topic}   当前速度: {speed}')
    try:
        while True:
            key = get_key(settings)
            twist = Twist()
            if key in MOVE:
                x, y, w = MOVE[key]
                twist.linear.x = x * speed
                twist.linear.y = y * speed
                twist.angular.z = w * speed * 2
            elif key == '+':
                speed = min(speed + 0.1, 2.0); print(f'速度: {speed:.1f}')
            elif key == '-':
                speed = max(speed - 0.1, 0.1); print(f'速度: {speed:.1f}')
            elif key == '\x03':  # Ctrl-C
                break
            pub.publish(twist)
    finally:
        pub.publish(Twist())  # 停车
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, settings)
        rclpy.shutdown()


if __name__ == '__main__':
    main()
