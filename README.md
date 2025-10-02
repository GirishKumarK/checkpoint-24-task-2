# ros2_ci — Jenkins CI for ROS 2 Humble + Gazebo (Headless)

This repository builds a Docker image with **ROS 2 Humble**, **Gazebo (Classic)**, the **FastBot** simulation packages, and your **fastbot_waypoints** package (Checkpoint 23).  
A Jenkins Pipeline builds the image and runs the tests automatically whenever a Pull Request is **merged** into the default branch.

---

## Repository URL (for Jenkins & PRs)
Use the URL of **this** repository in Jenkin:  
 `https://github.com/Andreas-Ioannou/checkpoint-24-task-2.git`

---

## 1) Start Jenkins

```bash
cd ~/webpage_ws
bash start_jenkins.sh
# Open the URL printed by the script (often http://localhost:8080)
# Initial admin password:
cat ~/jenkins_home/secrets/initialAdminPassword
# Minimal setup: continue as admin; suggested plugins (or None) are fine

2) Create the Jenkins Pipeline

Jenkins → New Item → Pipeline → name: ros2_ci.

Pipeline → Definition: Pipeline script from SCM

SCM: Git

Repository URL: (this repository’s URL)

Credentials: none (if public)

Script Path: Jenkinsfile

Save. The Jenkinsfile already contains pollSCM('* * * * *') (poll each minute).