#include <geometry_msgs/Point.h>
#include <geometry_msgs/PointStamped.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <ros/ros.h>
#include <visualization_msgs/Marker.h>

#include <Eigen/Eigen>

ros::Publisher marker_pub, expanded_marker_pub;
std::vector<Eigen::Vector2d> points, expanded_points;

void publishMaker(const ros::Publisher& pub,
                  const std::vector<Eigen::Vector2d>& points,
                  const Eigen::Vector4f color) {
  visualization_msgs::Marker marker;
  marker.header.frame_id = "map";
  marker.header.stamp = ros::Time::now();
  marker.ns = "basic_points";
  marker.id = 0;
  marker.type = visualization_msgs::Marker::POINTS;
  marker.action = visualization_msgs::Marker::ADD;

  marker.pose.orientation.x = 0.0;
  marker.pose.orientation.y = 0.0;
  marker.pose.orientation.z = 0.0;
  marker.pose.orientation.w = 1.0;

  marker.color.r = color[0];
  marker.color.g = color[1];
  marker.color.b = color[2];
  marker.color.a = color[3];

  marker.scale.x = 0.03;
  marker.scale.y = 0.03;
  marker.scale.z = 0.03;

  geometry_msgs::Point point;
  for (int i = 0; i < points.size(); i++) {
    point.x = points[i].x();
    point.y = points[i].y();
    marker.points.push_back(point);
  }

  pub.publish(marker);

  if (points.size() < 2) {
    return;
  }

  visualization_msgs::Marker line_marker;
  line_marker.header.frame_id = "map";
  line_marker.header.stamp = ros::Time::now();
  line_marker.ns = "basic_lines";
  line_marker.id = 1;
  line_marker.type = visualization_msgs::Marker::LINE_LIST;
  line_marker.action = visualization_msgs::Marker::ADD;

  line_marker.pose.orientation.x = 0.0;
  line_marker.pose.orientation.y = 0.0;
  line_marker.pose.orientation.z = 0.0;
  line_marker.pose.orientation.w = 1.0;

  line_marker.color.r = color[0];
  line_marker.color.g = color[1];
  line_marker.color.b = color[2];
  line_marker.color.a = color[3];
  line_marker.scale.x = 0.01;
  marker.scale.y = 0.01;
  marker.scale.z = 0.01;

  for (int i = 0; i < points.size() - 1; i++) {
    point.x = points[i].x();
    point.y = points[i].y();
    line_marker.points.push_back(point);

    point.x = points[i + 1].x();
    point.y = points[i + 1].y();
    line_marker.points.push_back(point);
  }

  point.x = points.back().x();
  point.y = points.back().y();
  line_marker.points.push_back(point);

  point.x = points.front().x();
  point.y = points.front().y();
  line_marker.points.push_back(point);

  pub.publish(line_marker);
}

// 注意：这个函数在输入点为顺时针排序时效果不太理想，逆时针排序则变成内缩。
void expandPointsSimple(const std::vector<Eigen::Vector2d>& points,
                        const double& expand_distance,
                        std::vector<Eigen::Vector2d>& expanded_points) {
  expanded_points.clear();
  int n = points.size();
  for (int i = 0; i < n; ++i) {
    Eigen::Vector2d p1 = points[i];
    Eigen::Vector2d p2 = points[(i + 1) % n];
    Eigen::Vector2d edge = p2 - p1;

    Eigen::Vector2d normal = Eigen::Vector2d(-edge.y(), edge.x()).normalized();
    normal *= expand_distance;

    Eigen::Vector2d expanded_point =
        Eigen::Vector2d(p1.x() + normal.x(), p1.y() + normal.y());
    expanded_points.push_back(expanded_point);
  }
}

void pointCallback(const geometry_msgs::PointStamped::ConstPtr& msg) {
  ROS_INFO("Received point: (%f, %f, %f)", msg->point.x, msg->point.y,
           msg->point.z);
  points.push_back({msg->point.x, msg->point.y});
  publishMaker(marker_pub, points, {1.0, 0.0, 0.0, 1.0});
}

void goalCallback(const geometry_msgs::PoseStamped::ConstPtr& msg) {
  ROS_INFO("Received goal");
  expandPointsSimple(points, 0.15, expanded_points);
  publishMaker(expanded_marker_pub, expanded_points, {0.0, 1.0, 0.0, 1.0});
}

void initialCallback(
    const geometry_msgs::PoseWithCovarianceStamped::ConstPtr& msg) {
  ROS_INFO("Received initial, clean points");
  points.clear();
  publishMaker(marker_pub, points, {1.0, 0.0, 0.0, 1.0});
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "geometry_node");
  ros::NodeHandle n;
  marker_pub = n.advertise<visualization_msgs::Marker>("/marker", 10);
  expanded_marker_pub =
      n.advertise<visualization_msgs::Marker>("/expanded_marker", 10);
  ros::Subscriber point_sub = n.subscribe("/clicked_point", 10, pointCallback);
  ros::Subscriber goal_sub =
      n.subscribe("/move_base_simple/goal", 10, goalCallback);
  ros::Subscriber initial_sub =
      n.subscribe("/initialpose", 10, initialCallback);

  ros::spin();
  return 0;
}
