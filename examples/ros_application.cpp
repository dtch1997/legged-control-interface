#include "types.h"
#include "hardware_interface.h"
#include "controller_interface.h"

#include "ros_hardware.h"
#include "sine_controller.h"
#include <iostream>
#include "utils.h"

#include <chrono>
#include <thread>
#include <signal.h>

void mySigintHandler(int sig)
{
    ros::shutdown();
    exit(0);
}

int main(int argc, char* argv[]) 
{

    ros::init(argc, argv, "ros_hardware", ros::init_options::NoSigintHandler);
    ros::NodeHandle nh;
    signal(SIGINT, mySigintHandler);

    interfaces::ControllerInterface* controller = new SineController();
    interfaces::HardwareInterface* hardware = new RosHardware(&nh);
    int num_iters;
    int loop_time_ms;

    char * arg_num_iters = getCmdOption(argv, argv+argc, "--num-iters");
    if (arg_num_iters == nullptr)
    {
        num_iters = 3;
    }
    else
    {
        num_iters = std::stoi(arg_num_iters);
    }

    char * arg_loop_time_ms = getCmdOption(argv, argv+argc, "--dt-ms");
    if (arg_loop_time_ms== nullptr)
    {
        loop_time_ms = 2;
    }
    else
    {
        loop_time_ms = std::stoi(arg_loop_time_ms);
    }
    std::cout << "[RosApp] Starting ros hardware" << std::endl;
    hardware->start();
    std::cout << "[RosApp] Ros hardware started" << std::endl;
    controller->start();

    // Initialize static memory
    static interfaces::RobotState robotState; 
    static interfaces::RobotCommand robotCommand;

    auto start = std::chrono::system_clock::now();
    // Demo control loop
    std::cout << "[RosApp] Starting loop" << std::endl;
    for (int i =0; i < num_iters; i ++) 
    {
        auto loop_start = std::chrono::system_clock::now();
        
        hardware->read();
        ros::spinOnce();
        hardware->getRobotState(robotState);
        controller->setRobotState(robotState);
        controller->step_control();
        controller->getRobotCommand(robotCommand);
        hardware->setRobotCommand(robotCommand);
        hardware->write();
        ros::spinOnce();

        auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(loop_start - start).count();
        std::cout << "[RosApp] Loop: " << i << " | Time: " << time_elapsed << " ms" << std::endl;
        std::this_thread::sleep_until(loop_start + std::chrono::milliseconds(loop_time_ms));
    }

    hardware->stop();
    controller->stop();
    std::cout << "[RosApp] Ros application terminated." << std::endl;

    return 0;
}