#include "rclcpp/rclcpp.hpp"
#include "trajectory_msgs/msg/joint_trajectory.hpp"
#include "trajectory_msgs/msg/joint_trajectory_point.hpp"
#include <cmath>
#include <chrono>

class JointPublisher : public rclcpp::Node
{
    public:
        JointPublisher()
        : Node("joint_publisher"), count_(0)
        {
            publisher_ = this->create_publisher<trajectory_msgs::msg::JointTrajectory>(
                "/joint_trajectory_controller/joint_trajectory", 10);
            timer_ = this->create_wall_timer(
                std::chrono::milliseconds(100), std::bind(&JointPublisher::timer_callback, this));
        }
    private:
        void timer_callback()
        {
            if(count_ % 10 == 0){
                auto message = trajectory_msgs::msg::JointTrajectory();

                message.joint_names.push_back("MotorLidar");

                auto point = trajectory_msgs::msg::JointTrajectoryPoint();
                //double position1 = 1.20*cos(count_/(20/3.14159))-0.1155408;
                double position1 = 0.5422295*cos(count_/(20/3.14159))+0.5422295;
                point.positions.push_back(position1);
                point.time_from_start = rclcpp::Duration::from_seconds(1.0);
                message.points.push_back(point);
                publisher_->publish(message);

                RCLCPP_INFO(this->get_logger(),"Publishing: '%f'", position1);
            }
            count_ +=1;
        }

        rclcpp::TimerBase::SharedPtr timer_;
        rclcpp::Publisher<trajectory_msgs::msg::JointTrajectory>::SharedPtr publisher_;
        size_t count_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<JointPublisher>());
    rclcpp::shutdown();
    return 0;
}