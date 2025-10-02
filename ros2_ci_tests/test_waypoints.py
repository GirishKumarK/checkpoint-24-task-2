import os
import time
import subprocess
import pytest

LAUNCH_CMD = [
    "bash", "-lc",
    "source /opt/ros/humble/setup.bash && "
    "source /ws/install/setup.bash && "
    "ros2 launch fastbot_gazebo one_fastbot_room.launch.py"
]


@pytest.fixture(scope="module", autouse=True)
def sim():
    # start sim
    proc = subprocess.Popen(
        LAUNCH_CMD, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    time.sleep(20)  # give sim time
    yield
    proc.terminate()
    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        proc.kill()


def test_waypoint_action_server_available():
    cmd = "source /opt/ros/humble/setup.bash && source /ws/install/setup.bash && ros2 node list"
    out = subprocess.check_output(
        ["bash", "-lc", cmd], text=True, stderr=subprocess.STDOUT)
    assert 'waypoints' in out or 'fastbot_waypoints' in out
