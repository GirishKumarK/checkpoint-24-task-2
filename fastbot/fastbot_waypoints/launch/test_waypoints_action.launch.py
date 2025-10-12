# fastbot_waypoints/launch/waypoints_action_with_client.launch.py
import os
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction, RegisterEventHandler, EmitEvent
from launch.event_handlers import OnProcessExit
from launch.events import Shutdown
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    gz_launch = os.path.join(
        get_package_share_directory('fastbot_gazebo'),
        'launch', 'one_fastbot_room.launch.py'
    )
    gazebo = IncludeLaunchDescription(PythonLaunchDescriptionSource(gz_launch))

    server = Node(
        package='fastbot_waypoints',
        executable='fastbot_action_server',
        name='fastbot_waypoints_server',
        output='screen',
    )

    client = Node(
        package='fastbot_waypoints',
        executable='test_waypoints',
        name='fastbot_waypoints_client',
        output='screen',
        arguments=['--gtest_color=yes'],
    )

    server_after = TimerAction(period=15.0, actions=[server])
    client_after = TimerAction(period=10.0, actions=[client])

    # Delay shutdown by 2 seconds after client exits
    shutdown_when_client_exits = RegisterEventHandler(
        OnProcessExit(
            target_action=client,
            on_exit=[
                TimerAction(period=7.0, actions=[EmitEvent(event=Shutdown())])
            ]
        )
    )

    return LaunchDescription([
        gazebo,
        server_after,
        client_after,
        shutdown_when_client_exits,
    ])
