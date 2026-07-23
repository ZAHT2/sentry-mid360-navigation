# GitHub Publishing Checklist

This file records the suggested public-release process for this workspace.

## 1. Prepare Repository

```bash
cd /home/zaht/sentry_nav_ws
git status --short
```

Only source code, launch files, configs, small 2D map examples, and Markdown documents should be committed.

Do not commit generated or large runtime data:

```text
build/
install/
log/
bags/
real_maps/*.pcd
*.zip
*.tar.gz
```

These are already covered by `.gitignore`.

## 2. Optional Large Assets

If a public demo needs a matching 3D prior cloud, upload the PCD file to GitHub Releases instead of the git tree.

Recommended naming:

```text
real_scans15.pcd
real_map15.yaml
real_map15.pgm
real_map15.png
```

Keep the YAML/PGM/PNG in git if they are small enough. Keep PCD outside git.

## 3. Build Check Before Push

```bash
cd /home/zaht/sentry_nav_ws
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
```

## 4. First Commit

```bash
cd /home/zaht/sentry_nav_ws
git add README.md GITHUB_PUBLISHING.md .gitignore \
  mid360_宿主机建图流程.md mid360_导航固定流程.md \
  firmware_nav2_cmdvel_patch.md ISSUES.md \
  maps real_maps src test_serial key_teleop.py

git status --short
git commit -m "Initial public release of host MID360 navigation workspace"
```

Before committing, inspect `git status --short` carefully. Do not stage ignored large files with `git add -f`.

## 5. Create GitHub Remote

Create an empty GitHub repository, then run:

```bash
git remote add origin git@github.com:<your-user>/<your-repo>.git
git push -u origin main
```

If using HTTPS:

```bash
git remote add origin https://github.com/<your-user>/<your-repo>.git
git push -u origin main
```

## 6. Public Description

Suggested repository description:

```text
ROS 2 Humble host-side MID360 Point-LIO/GICP/Nav2 sentry robot mapping and low-speed navigation workspace.
```
