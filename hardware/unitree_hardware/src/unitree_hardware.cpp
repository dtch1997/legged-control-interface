#include "unitree_hardware.h"
#include <sys/mman.h>

#define MAX_STACK_SIZE 16384
#define TASK_PRIORITY 49 // 49

// Public API

int UnitreeHardware::start()
{
    running_ = true;
    comm_ready_ = true;

    printf ( "[HardwareBridge] Init stack\n" );
    PrefaultStack();
    printf ( "[HardwareBridge] Init scheduler\n" );
    SetupScheduler();
    SetInitValue();

    udp.InitCmdData(cmd_);
    udp.SetSend(cmd_);
    udp.GetRecv(data_);
    return 0;
    // TODO: Allow some time to warm up
}

int UnitreeHardware::stop()
{
    running_ = false;
    return 0;
}

int UnitreeHardware::read() 
{
    // Update data_;
    udp.GetRecv(data_);
    udp.Recv();

    // Update _robot_state.legs
    for (uint leg = 0; leg<4; leg++)
    {
        for(uint jindx = 0; jindx<3; jindx++)
        {
            _robot_state.legs[leg].joints[jindx].q = data_.motorState[leg*3+jindx].q;
            _robot_state.legs[leg].joints[jindx].qd = data_.motorState[leg*3+jindx].dq;
            _robot_state.legs[leg].joints[jindx].tau = data_.motorState[leg*3+jindx].tauEst;
        }
    }

    // Update _robot_state.imu
    for (uint i = 0; i < 4; i++)
    {
        _robot_state.imu.quad[i] = data_.imu.quaternion[i];
    }
    for (uint i = 0; i < 3; i++)
    {
        _robot_state.imu.gyro[i] = data_.imu.gyroscope[i];
    }
    for (uint i = 0; i < 3; i++)
    {
        _robot_state.imu.acc[i] = data_.imu.accelerometer[i];
    }

    return 0;
}

int UnitreeHardware::write() 
{
    // Update cmd_ from _robot_command
    for (uint leg = 0; leg<4; leg++)
    {
        for(uint jindx = 0; jindx<3; jindx++)
        {
            cmd_.motorCmd[leg*3+jindx].q = _robot_command.legs[leg].joints[jindx].q;
            cmd_.motorCmd[leg*3+jindx].dq = _robot_command.legs[leg].joints[jindx].qd;
            cmd_.motorCmd[leg*3+jindx].tau = _robot_command.legs[leg].joints[jindx].tau;
            cmd_.motorCmd[leg*3+jindx].Kp = _robot_command.legs[leg].joints[jindx].kp;
            cmd_.motorCmd[leg*3+jindx].Kd = _robot_command.legs[leg].joints[jindx].kd;
        }
    }

    // Apply safety limit
    safe.PositionLimit(cmd_);
    safe.PowerProtect(cmd_, data_, 10);

    // For debugging:
    // printf("[UnitreeHardware] FR knee qDes: %f\n", cmd_.motorCmd[1].q);
    
    // Send cmd_ to UDP
    udp.SetSend(cmd_);
    udp.Send();

    return 0;
}  

// Private functions

void InitError ( const char* reason, bool printErrno )
{
    printf ( "FAILED TO INITIALIZE HARDWARE: %s\n", reason );
    if ( printErrno ) {
        // printf("Error: %s\n", strerror(errno));
    }
    exit ( -1 );
}

void UnitreeHardware::PrefaultStack()
{
    printf ( "[Init] Prefault stack...\n" );
    volatile char stack[MAX_STACK_SIZE];
    memset ( const_cast<char*> ( stack ), 0, MAX_STACK_SIZE );
    if ( mlockall ( MCL_CURRENT | MCL_FUTURE ) == -1 ) {
        InitError (
            "mlockall failed.  This is likely because you didn't run robot as "
            "root.\n",
            true );
    }
}

void UnitreeHardware::SetupScheduler()
{
    printf ( "[Init] Setup RT Scheduler...\n" );
    struct sched_param params;
    params.sched_priority = TASK_PRIORITY;
    if ( sched_setscheduler ( 0, SCHED_FIFO, &params ) == -1 ) {
        InitError ( "sched_setscheduler failed.\n", true );
    }
}

void UnitreeHardware::SetInitValue()
{
    for (uint leg = 0; leg<4; leg++){
        for(uint jindx = 0; jindx<3; jindx++)
        {
            cmd_.motorCmd[leg*3+jindx].q = 0.0;
            cmd_.motorCmd[leg*3+jindx].dq = 0.0;
            cmd_.motorCmd[leg*3+jindx].tau = 0.0;
            cmd_.motorCmd[leg*3+jindx].Kp = 0.0;
            cmd_.motorCmd[leg*3+jindx].Kd = 0.0;
        }
    }

    for (uint leg = 0; leg<4; leg++){
        for(uint jindx = 0; jindx<3; jindx++)
        {
            data_.motorState[leg*3+jindx].q = 0.0;
            data_.motorState[leg*3+jindx].dq = 0.0;
            data_.motorState[leg*3+jindx].tauEst = 0.0;
        }
    }
}