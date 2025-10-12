# ros2_ci — Jenkins CI for ROS2 (Humble) + Gazebo

## This repository provides a Dockerized ROS2 Humble + Gazebo environment and a Jenkins Pipeline that builds the image and runs tests automatically whenever a Pull Request is merged into the default branch.

# Repository URL (for Jenkins & PRs):
https://github.com/Andreas-Ioannou/checkpoint-24-task-2.git

# 1) What’s inside

- Dockerfile — Builds an image with ROS2 Humble, Gazebo, your simulation packages, and tests.

## Jenkins — Pipeline that:

- checks out this repo

- builds the Docker image

- runs tests

- publishes JUnit XML results

# 3) Start Jenkins

- cd ~/webpage_ws
- bash start_jenkins.sh
- cd
- #Open the "jenkins__pid__url.txt" and access the URL
# Get initial admin password (current password):
cat ~/jenkins_home/secrets/initialAdminPassword

- Username: admin
- Password: c3d95b993e2e4a2bbafac7777f06f1e0