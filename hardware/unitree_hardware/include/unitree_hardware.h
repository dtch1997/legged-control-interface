#include "hardware_interface.h"
#include "unitree_legged_sdk/unitree_legged_sdk.h"

class UnitreeHardware : public interfaces::HardwareInterface
{
public:
    int start() override; 
    int read() override;
    int write() override; 
    int stop() override;

private:
    void PrefaultStack();
    void SetupScheduler();
    void SetInitValue();

    UNITREE_LEGGED_SDK::Safety safe;
    UNITREE_LEGGED_SDK::UDP udp;

    bool comm_ready_;
    bool running_;

    UNITREE_LEGGED_SDK::LowCmd cmd_ = {0};
    UNITREE_LEGGED_SDK::LowState data_ = {0};;
};