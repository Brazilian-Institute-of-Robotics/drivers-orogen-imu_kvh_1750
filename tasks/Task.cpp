/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <fog_kvh_1750/Driver.hpp>

using namespace fog_kvh_1750;

Task::Task(std::string const& name)
    : TaskBase(name)
{
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
}

Task::~Task()
{
}

fog_kvh_1750::Driver fog_driver;
fog_acceleration acc;

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (! TaskBase::configureHook())
        return false;
    return true;
}
bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;

    fog_driver.open(_device);

    return true;
}
void Task::updateHook()
{
    TaskBase::updateHook();

    fog_driver.read();

    acc = fog_driver.getAcceleration();
    std::cout << "acc.x = " << acc.x << std::endl;
    _acceleration.write(acc);
}

void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    TaskBase::stopHook();
}
void Task::cleanupHook()
{
    TaskBase::cleanupHook();
}
