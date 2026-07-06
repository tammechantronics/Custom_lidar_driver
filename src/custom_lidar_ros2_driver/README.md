# custom_lidar_ros2_driver

ROS2 wrapper cho `custom_lidar_sdk`.

Node:

```text
custom_lidar_ros2_driver_node
```

Publish:

```text
/scan : sensor_msgs/msg/LaserScan
```

Services:

```text
/start_scan : std_srvs/srv/Empty
/stop_scan  : std_srvs/srv/Empty
```
