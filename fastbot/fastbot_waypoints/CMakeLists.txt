cmake_minimum_required(VERSION 3.8)
project(fastbot_waypoints)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core deps
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(std_msgs REQUIRED)

# --------- Interfaces: generate FIRST ----------
find_package(rosidl_default_generators REQUIRED)
rosidl_generate_interfaces(${PROJECT_NAME}
  "action/WaypointAction.action"
  DEPENDENCIES geometry_msgs std_msgs
)
ament_export_dependencies(rosidl_default_runtime)

install(DIRECTORY
  launch
  test
  DESTINATION share/${PROJECT_NAME}/
)

# Add the generated headers dir globally for all targets in this package
# (some setups still miss target-scoped includes; this makes it foolproof)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp)

# --------- Node ----------
add_executable(fastbot_action_server src/fastbot_action_server.cpp)
ament_target_dependencies(fastbot_action_server
  rclcpp rclcpp_action geometry_msgs nav_msgs tf2 tf2_geometry_msgs tf2_ros
)
# Enforce build order and add build/install include paths
add_dependencies(fastbot_action_server ${PROJECT_NAME}__rosidl_generator_cpp)
target_include_directories(fastbot_action_server PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp>
  $<INSTALL_INTERFACE:include>
)
# Wire in typesupport (ensures interface targets are linked/exported)
rosidl_target_interfaces(fastbot_action_server ${PROJECT_NAME} "rosidl_typesupport_cpp")

install(TARGETS fastbot_action_server
  DESTINATION lib/${PROJECT_NAME})

# --------- Tests ----------
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_waypoints test/test_waypoints.cpp)
  if(TARGET test_waypoints)
    ament_target_dependencies(test_waypoints
      rclcpp rclcpp_action geometry_msgs nav_msgs tf2 tf2_geometry_msgs tf2_ros
    )
    add_dependencies(test_waypoints ${PROJECT_NAME}__rosidl_generator_cpp)
    target_include_directories(test_waypoints PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp>
      $<INSTALL_INTERFACE:include>
    )
    rosidl_target_interfaces(test_waypoints ${PROJECT_NAME} "rosidl_typesupport_cpp")

	install(TARGETS test_waypoints
      RUNTIME DESTINATION lib/${PROJECT_NAME}
    )
  endif()
endif()

ament_package()
