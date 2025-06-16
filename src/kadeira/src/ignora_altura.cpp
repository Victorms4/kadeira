#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <cmath>

class LidarHeightFilter : public rclcpp::Node
{
public:
    LidarHeightFilter()
        : Node("lidar_height_filter"), lidar_angle_(0.0), lidar_height_(1.5475) // Altura do LIDAR
    {
        // Assinando os tópicos scan e joint_states
        scan_subscription_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
            "scan", 10, std::bind(&LidarHeightFilter::scan_callback, this, std::placeholders::_1));

        joint_state_subscription_ = this->create_subscription<sensor_msgs::msg::JointState>(
            "joint_states", 10, std::bind(&LidarHeightFilter::joint_state_callback, this, std::placeholders::_1));

        scan_publisher_ = this->create_publisher<sensor_msgs::msg::LaserScan>("scan_filtered", 10);
    }

private:
    void joint_state_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
    {
        // Encontrar a junta "MotorLidar" e pegar a posição (ângulo da junta)
        for (size_t i = 0; i < msg->name.size(); ++i)
        {
            if (msg->name[i] == "MotorLidar")
            {
                lidar_angle_ = msg->position[i]; // Atualiza o ângulo atual da junta
                RCLCPP_INFO(this->get_logger(), "Ângulo da junta (radianos): %.2f", lidar_angle_);
                break;
            }
        }
    }

    void scan_callback(const sensor_msgs::msg::LaserScan::SharedPtr msg)
    {
        auto filtered_scan = *msg; // Copiando a mensagem original para modificá-la

        // Imprimir o cabeçalho da mensagem LaserScan
        RCLCPP_INFO(this->get_logger(), "Cabeçalho LaserScan: frame_id = %s, stamp = %u.%u", 
                    msg->header.frame_id.c_str(), msg->header.stamp.sec, msg->header.stamp.nanosec);

        // Calcular a distância do LIDAR ao chão (hipotenusa)
        double lidar_to_floor_distance = lidar_height_ / std::sin(lidar_angle_);

        // Usar 95% dessa distância para descartar leituras
        double threshold_distance = lidar_to_floor_distance * 0.95;

        size_t num_readings = msg->ranges.size();
        double angle_increment = msg->angle_increment;
        double current_angle = msg->angle_min;

        for (size_t i = 0; i < num_readings; ++i)
        {
            float distance = msg->ranges[i]; // Distância lida pelo LIDAR

            // Verificar se a leitura está fora dos limites válidos
            if (std::isnan(distance) || distance < msg->range_min || distance > msg->range_max)
            {
                current_angle += angle_increment;
                continue; // Ignorar leituras inválidas
            }

            // Aplicar o filtro apenas se a leitura estiver dentro do cone de 70 graus (±35 graus)
            //if (current_angle >= -M_PI * 70.0 / 180.0 && current_angle <= M_PI * 70.0 / 180.0) // Ângulo entre -70° e +70°
            //{
                if (distance > threshold_distance)
                {
                    filtered_scan.ranges[i] = std::numeric_limits<float>::quiet_NaN(); // Marca a leitura como inválida
                }
            //}

            current_angle += angle_increment;
        }

        // Publica as leituras filtradas no novo tópico
        scan_publisher_->publish(filtered_scan);
    }

    rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_subscription_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_subscription_;
    rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr scan_publisher_;

    double lidar_angle_;  // Armazena o ângulo da junta do LIDAR
    const double lidar_height_; // Altura fixa do LIDAR
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<LidarHeightFilter>());
    rclcpp::shutdown();
    return 0;
}
