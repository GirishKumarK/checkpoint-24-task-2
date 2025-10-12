### Build the packages
cd ros2_ws
colcon build
source install/setup.bash

### Launch the simulation in gazebo

source ~/ros2_ws/install/setup.bash
ros2 launch fastbot_gazebo one_fastbot_room.launch.py

### Launch action server
## In another terminal 

source ~/ros2_ws/install/setup.bash
ros2 run fastbot_waypoints fastbot_action_server

## Expected output: 

user:~/ros2_ws$ colcon test --packages-select fastbot_waypoints --event-handler=console_direct+   --ctest-args -R test_waypoints -VV
Starting >>> fastbot_waypoints
UpdateCTestConfiguration  from :/home/user/ros2_ws/build/fastbot_waypoints/CTestConfiguration.ini
Parse Config file:/home/user/ros2_ws/build/fastbot_waypoints/CTestConfiguration.ini
   Site: 1_xterm
   Build name: (empty)
 Add coverage exclude regular expressions.
SetCTestConfiguration:CMakeCommand:/usr/bin/cmake
Create new tag: 20250822-1339 - Experimental
UpdateCTestConfiguration  from :/home/user/ros2_ws/build/fastbot_waypoints/CTestConfiguration.ini
Parse Config file:/home/user/ros2_ws/build/fastbot_waypoints/CTestConfiguration.ini
Test project /home/user/ros2_ws/build/fastbot_waypoints
Constructing a list of tests
Done constructing a list of tests
Updating test list for fixtures
Added 0 tests to meet fixture requirements
Checking test dependency graph...
Checking test dependency graph end
test 1
    Start 1: test_waypoints

1: Test command: /usr/bin/python3 "-u" "/opt/ros/galactic/share/ament_cmake_test/cmake/run_test.py" "/home/user/ros2_ws/build/fastbot_waypoints/test_results/fastbot_waypoints/test_waypoints.gtest.xml" "--package-name" "fastbot_waypoints" "--output-file" "/home/user/ros2_ws/build/fastbot_waypoints/ament_cmake_gtest/test_waypoints.txt" "--command" "/home/user/ros2_ws/build/fastbot_waypoints/test_waypoints" "--gtest_output=xml:/home/user/ros2_ws/build/fastbot_waypoints/test_results/fastbot_waypoints/test_waypoints.gtest.xml"
1: Test timeout computed to be: 60
1: -- run_test.py: invoking following command in '/home/user/ros2_ws/build/fastbot_waypoints':
1:  - /home/user/ros2_ws/build/fastbot_waypoints/test_waypoints --gtest_output=xml:/home/user/ros2_ws/build/fastbot_waypoints/test_results/fastbot_waypoints/test_waypoints.gtest.xml
1: Running main() from /opt/ros/galactic/src/gtest_vendor/src/gtest_main.cc
1: [==========] Running 1 test from 1 test suite.
1: [----------] Global test environment set-up.
1: [----------] 1 test from FastbotDistanceStopTest
1: [ RUN      ] FastbotDistanceStopTest.DistanceReachedAndStopped
1: [       OK ] FastbotDistanceStopTest.DistanceReachedAndStopped (0 ms)
1: [----------] 1 test from FastbotDistanceStopTest (0 ms total)
1:
1: [----------] Global test environment tear-down
1: [==========] 1 test from 1 test suite ran. (5185 ms total)
1: [  PASSED  ] 1 test.
1: -- run_test.py: return code 0
1: -- run_test.py: inject classname prefix into gtest result file '/home/user/ros2_ws/build/fastbot_waypoints/test_results/fastbot_waypoints/test_waypoints.gtest.xml'
1: -- run_test.py: verify result file '/home/user/ros2_ws/build/fastbot_waypoints/test_results/fastbot_waypoints/test_waypoints.gtest.xml'
1/1 Test #1: test_waypoints ...................   Passed    5.29 sec

The following tests passed:
        test_waypoints

100% tests passed, 0 tests failed out of 1

Label Time Summary:
gtest    =   5.29 sec*proc (1 test)

Total Test time (real) =   5.29 sec
Finished <<< fastbot_waypoints [5.37s]

Summary: 1 package finished [5.67s]


### Run the test (PASS example)

export FASTBOT_TARGET_DX=0.25 FASTBOT_TARGET_DY=0.0
colcon test --packages-select fastbot_waypoints --event-handler=console_direct+   --ctest-args -R test_waypoints -VV

### Run the test (FAILURE example), (the robot never reaches the desired position, crashes into an obstacle)

export FASTBOT_TARGET_DX=3.0 FASTBOT_TARGET_DY=0.0
colcon test --packages-select fastbot_waypoints --event-handler=console_direct+   --ctest-args -R test_waypoints -VV

## Expected output:

Starting >>> fastbot_waypoints
UpdateCTestConfiguration  from :/home/user/ros2_ws/build/fastbot_waypoints/CTestConfiguration.ini
Parse Config file:/home/user/ros2_ws/build/fastbot_waypoints/CTestConfiguration.ini
   Site: 1_xterm
   Build name: (empty)
 Add coverage exclude regular expressions.
SetCTestConfiguration:CMakeCommand:/usr/bin/cmake
Create new tag: 20250822-1312 - Experimental
UpdateCTestConfiguration  from :/home/user/ros2_ws/build/fastbot_waypoints/CTestConfiguration.ini
Parse Config file:/home/user/ros2_ws/build/fastbot_waypoints/CTestConfiguration.ini
Test project /home/user/ros2_ws/build/fastbot_waypoints
Constructing a list of tests
Done constructing a list of tests
Updating test list for fixtures
Added 0 tests to meet fixture requirements
Checking test dependency graph...
Checking test dependency graph end
test 1
    Start 1: test_waypoints

1: Test command: /usr/bin/python3 "-u" "/opt/ros/galactic/share/ament_cmake_test/cmake/run_test.py" "/home/user/ros2_ws/build/fastbot_waypoints/test_results/fastbot_waypoints/test_waypoints.gtest.xml" "--package-name" "fastbot_waypoints" "--output-file" "/home/user/ros2_ws/build/fastbot_waypoints/ament_cmake_gtest/test_waypoints.txt" "--command" "/home/user/ros2_ws/build/fastbot_waypoints/test_waypoints" "--gtest_output=xml:/home/user/ros2_ws/build/fastbot_waypoints/test_results/fastbot_waypoints/test_waypoints.gtest.xml"
1: Test timeout computed to be: 60
1: -- run_test.py: invoking following command in '/home/user/ros2_ws/build/fastbot_waypoints':
1:  - /home/user/ros2_ws/build/fastbot_waypoints/test_waypoints --gtest_output=xml:/home/user/ros2_ws/build/fastbot_waypoints/test_results/fastbot_waypoints/test_waypoints.gtest.xml
1: Running main() from /opt/ros/galactic/src/gtest_vendor/src/gtest_main.cc
1: [==========] Running 1 test from 1 test suite.
1: [----------] Global test environment set-up.
1: [----------] 1 test from FastbotDistanceStopTest
[Processing: fastbot_waypoints]
[Processing: fastbot_waypoints]
1/1 Test #1: test_waypoints ...................***Timeout  60.06 sec

0% tests passed, 1 tests failed out of 1

Label Time Summary:
gtest    =  60.06 sec*proc (1 test)

Total Test time (real) =  60.06 sec

The following tests FAILED:
          1 - test_waypoints (Timeout)
Errors while running CTest
--- stderr: fastbot_waypoints
Errors while running CTest
---
Finished <<< fastbot_waypoints [1min 0s]        [ with test failures ]
