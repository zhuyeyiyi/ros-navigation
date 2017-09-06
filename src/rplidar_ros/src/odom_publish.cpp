#include <ros/ros.h>
#include <tf/transform_broadcaster.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/Float32.h>

#define WHEEL_DISTANCE 0.55 // d denotes the distance between both wheels

double v_left=0.0;
double v_right=0.0;
double v_rx, v_ry, v_wx, v_wy, omega_r, theta, thetadot;

double cmd_vel_lwheel = 0.0;
double cmd_vel_rwheel = 0.0;


void velcmd_callback(const geometry_msgs::Twist::ConstPtr & vel_msg){
    double cmd_velocity = vel_msg->linear.x;
    double cmd_anglevel = vel_msg->angular.z;

    double rwheel_vel = 0;
    double lwheel_vel = 0;

    if( cmd_velocity == 0){
        rwheel_vel = cmd_anglevel*WHEEL_DISTANCE/2.0;
        rwheel_vel = (-1.0)*rwheel_vel;
    }
    else if( cmd_anglevel == 0 ){
        rwheel_vel = cmd_velocity;
        lwheel_vel = cmd_velocity;
    }
    else{
        rwheel_vel = cmd_velocity + cmd_anglevel*WHEEL_DISTANCE/2.0;
        lwheel_vel = cmd_velocity - cmd_anglevel*WHEEL_DISTANCE/2.0;
    }

    v_left = lwheel_vel;
    v_right = rwheel_vel;
}


int main(int argc, char** argv) {
  ros::init(argc, argv, "odometry_publisher");
  
  ros::NodeHandle n;
  ros::Publisher odom_pub = n.advertise<nav_msgs::Odometry>("odom", 50);
  tf::TransformBroadcaster odom_broadcaster;
  ros::Subscriber r_sub = n.subscribe("cmd_vel", 50, velcmd_callback); 
  
  ros::Time current_time, last_time;
  current_time = ros::Time::now();
  last_time = ros::Time::now();
  
  //publish the odometry information at a rate of 10 Hz
  ros::Rate loop_rate(50);
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;

  while(n.ok()) {
    ros::spinOnce();
   //calculate odometry
    current_time = ros::Time::now();
    double dt = (current_time-last_time).toSec();
   // calculate velocities in the robot frame (which are located in the center of rotation, e.g. base_link frame)
    v_rx = (v_right+v_left)/2;
    v_ry = 0;  // we have a non-holonomic constraint (for a holonomic robot, this is non-zero)
    omega_r = (v_right-v_left)/WHEEL_DISTANCE;  
   // calculate Velocities in the odom frame (which is fixed-world and the same as the robot frame at the beginning.)
    v_wx = cos(theta)*v_rx-sin(theta)*v_ry;
    v_wy = sin(theta)*v_rx+cos(theta)*v_ry;
    thetadot = omega_r;
    x += v_wx*dt;
    y += v_wy*dt;
    theta += thetadot*dt;
    geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(theta);
   //publish the transform over tf
    geometry_msgs::TransformStamped odom_trans;
    odom_trans.header.stamp = current_time;
    odom_trans.header.frame_id = "odom";
    odom_trans.child_frame_id = "base_link";

    odom_trans.transform.translation.x = x;
    odom_trans.transform.translation.y = y;
    odom_trans.transform.translation.z = 0.0;
    odom_trans.transform.rotation = odom_quat;
   //send the transform
    odom_broadcaster.sendTransform(odom_trans);
  
   //publish the odometry message over ROS
    nav_msgs::Odometry odom;
    odom.header.stamp = current_time;
    odom.header.frame_id = "odom";
    odom.child_frame_id = "base_link";
   //set the position
    odom.pose.pose.position.x = x;
    odom.pose.pose.position.y = y;
    odom.pose.pose.position.z = 0.0;
    odom.pose.pose.orientation = odom_quat;
   //set the velocity
    odom.twist.twist.linear.x = v_rx;
    odom.twist.twist.linear.y = v_ry;
    odom.twist.twist.angular.z = omega_r;
   //publish the message
    odom_pub.publish(odom);
    last_time = current_time;
    loop_rate.sleep();
    }
}
