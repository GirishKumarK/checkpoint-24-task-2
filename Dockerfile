# ROS 2 Humble on Ubuntu 22.04
FROM ros:humble-desktop

SHELL ["/bin/bash","-lc"]
ENV DEBIAN_FRONTEND=noninteractive

# Base tools + Gazebo (Classic) bindings for ROS 2
RUN apt-get update -y && apt-get install -y --no-install-recommends \
    git curl ca-certificates \
    python3-pip python3-colcon-common-extensions python3-rosdep \
    ros-humble-gazebo-ros-pkgs ros-humble-gazebo-plugins \
    xvfb x11-apps \
 && rm -rf /var/lib/apt/lists/*

# Initialize rosdep
RUN rosdep init || true
RUN rosdep update

# Workspace
ENV ROS_WS=/root/ros2_ws
RUN mkdir -p $ROS_WS/src
WORKDIR $ROS_WS

# Bring in sources (your FastBot sim + fastbot_waypoints should be under ./src)
COPY src/ $ROS_WS/src/

# Install dependencies
RUN source /opt/ros/humble/setup.bash && \
    rosdep install --from-paths src --ignore-src -r -y

# Build (merge-install makes sourcing easy)
RUN source /opt/ros/humble/setup.bash && \
    colcon build --merge-install

# Environment
ENV ROS_DISTRO=humble
ENV ROS_DOMAIN_ID=7
ENV RMW_IMPLEMENTATION=rmw_fastrtps_cpp

# Entrypoint + helper scripts
COPY scripts/entrypoint.sh /entrypoint.sh
COPY scripts/run_sim.sh /run_sim.sh
COPY scripts/test_waypoints.sh /test_waypoints.sh
RUN chmod +x /entrypoint.sh /run_sim.sh /test_waypoints.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]
