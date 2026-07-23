# GitHub 发布流程

这份文档记录 `sentry_nav_ws` 发布到 GitHub 的固定流程。当前仓库：

```text
GitHub 用户: ZAHT2
远程仓库: sentry-mid360-navigation
远程地址: https://github.com/ZAHT2/sentry-mid360-navigation.git
```

## 1. 发布前检查

进入工作空间：

```bash
cd /home/zaht/sentry_nav_ws
git status --short
```

应该提交的内容：

```text
src/                      源码包、launch、参数
maps/                     小型示例 2D 地图
real_maps/*.yaml          实车 2D 地图元数据
real_maps/*.pgm           实车 2D 栅格图
real_maps/*.png           地图预览图
*.md                      README、建图 SOP、导航 SOP、发布说明
test_serial/              串口测试工具
```

不要提交的内容：

```text
build/
install/
log/
bags/
real_maps/*.pcd
*.zip
*.tar.gz
```

这些大文件或运行产物已经写入 `.gitignore`。不要用 `git add -f` 强行加入。

## 2. PCD 和大文件怎么发布

`real_scans*.pcd` 是 3D 点云地图，文件很大，不放进 git。

如果需要公开某一版完整地图，例如 `map15`，建议把下面文件作为 GitHub Release 附件：

```text
real_scans15.pcd
real_map15.yaml
real_map15.pgm
real_map15.png
```

其中 `yaml/pgm/png` 可以保留在 git；`pcd` 用 Release 或网盘。

## 3. 构建检查

发布前建议至少跑一次构建：

```bash
cd /home/zaht/sentry_nav_ws
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
```

如果只是改 Markdown，可以不重新构建。

## 4. 提交本地修改

第一次发布时：

```bash
cd /home/zaht/sentry_nav_ws

git config user.name "ZAHT2"
git config user.email "2427791923@qq.com"

git add .
git status --short
git commit -m "Initial public release of host MID360 navigation workspace"
```

日常更新时：

```bash
cd /home/zaht/sentry_nav_ws

git add .
git status --short
git commit -m "Update navigation docs and parameters"
```

提交前必须看一眼 `git status --short`，确认没有误加入 PCD、bag、zip、tar.gz。

## 5. 绑定远程仓库

如果还没有远程仓库：

```bash
cd /home/zaht/sentry_nav_ws
git remote add origin https://github.com/ZAHT2/sentry-mid360-navigation.git
```

如果已经有远程，只是确认：

```bash
git remote -v
```

当前正确输出应包含：

```text
origin  https://github.com/ZAHT2/sentry-mid360-navigation.git (fetch)
origin  https://github.com/ZAHT2/sentry-mid360-navigation.git (push)
```

## 6. 推送到 GitHub

执行：

```bash
cd /home/zaht/sentry_nav_ws
git push -u origin main
```

如果终端提示：

```text
Username for 'https://github.com':
```

输入：

```text
ZAHT2
```

如果终端提示：

```text
Password for 'https://ZAHT2@github.com':
```

这里不能填 GitHub 登录密码。GitHub 已经不支持密码推送，必须填 Personal Access Token。

## 7. 生成 GitHub Token

打开：

```text
https://github.com/settings/tokens
```

选择：

```text
Generate new token -> Generate new token (classic)
```

推荐设置：

```text
Note: sentry_nav_ws_push
Expiration: 30 days 或 90 days
Scopes: 勾选 repo
```

生成后复制 token。它只显示一次。

再次推送：

```bash
cd /home/zaht/sentry_nav_ws
git push -u origin main
```

填写：

```text
Username: ZAHT2
Password: 粘贴 token
```

终端粘贴 token 时不会显示字符，直接回车即可。

## 8. 成功判据

推送成功时终端会出现类似：

```text
Enumerating objects...
Writing objects...
To https://github.com/ZAHT2/sentry-mid360-navigation.git
   xxxxxxx..yyyyyyy  main -> main
branch 'main' set up to track 'origin/main'.
```

然后打开：

```text
https://github.com/ZAHT2/sentry-mid360-navigation
```

能看到 README、`src/`、两份 MID360 SOP 文档，就说明发布成功。

## 9. 仓库简介建议

GitHub 仓库 Description 可以写：

```text
ROS 2 Humble host-side MID360 Point-LIO/GICP/Nav2 sentry robot mapping and low-speed navigation workspace.
```

中文说明：

```text
基于 ROS 2 Humble、MID360、Point-LIO、small_gicp 和 Nav2 的宿主机实车建图与低速导航工作空间。
```

---

# English Quick Reference

This repository publishes the host-side ROS 2 Humble MID360 navigation workspace.

Repository:

```text
https://github.com/ZAHT2/sentry-mid360-navigation.git
```

Do commit:

```text
src/
maps/
real_maps/*.yaml
real_maps/*.pgm
real_maps/*.png
*.md
test_serial/
```

Do not commit:

```text
build/
install/
log/
bags/
real_maps/*.pcd
*.zip
*.tar.gz
```

Build check:

```bash
cd /home/zaht/sentry_nav_ws
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
```

Push:

```bash
cd /home/zaht/sentry_nav_ws
git push -u origin main
```

For HTTPS push, use:

```text
Username: ZAHT2
Password: GitHub Personal Access Token, not account password
```

