/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <imu_kvh_1750/Driver.hpp>
#include <base/samples/IMUSensors.hpp>
#include <base-logging/Logging.hpp>
#include <Eigen/Geometry>
#include <boost/numeric/conversion/cast.hpp>

using namespace imu_kvh_1750;

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

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    /*************************************/
    /** Configuration of Time estimator **/
    /*************************************/
    if(_sampling_frequency.value() <= 0.0)
    {
	RTT::log(RTT::Error) << "The sampling frequency has to be a positive non-zero value." << RTT::endlog();
	return false;
    }

    kvh_driver.reset(new imu_kvh_1750::Driver());
    setDriver(kvh_driver.get());

    if (! TaskBase::configureHook())
        return false;

    if (_io_port.value().empty())
        kvh_driver->open(_io_port.value());
    
    return true;
}

bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;

    timestamp_estimator.reset(
	base::Time::fromSeconds(20),
	base::Time::fromSeconds(1.0 / _sampling_frequency.value()),
	base::Time::fromSeconds(0),
	INT_MAX);

    return true;
}


void Task::processIO()
{
    kvh_driver->read();
    base::samples::IMUSensors imusamples = kvh_driver->getIMUReading();

    /** rotate measurments to the local frame */
    if(!_axes_orientation.value().isZero())
    {
        imusamples.acc = _axes_orientation.value() * imusamples.acc;
        imusamples.gyro = _axes_orientation.value() * imusamples.gyro;
    }

    /** acceleration in m/s^2 */
    imusamples.acc = imusamples.acc * GRAVITY_SI; //g to m/s^2, KVH puts out acceleration in g

    if(_gyroscope_delta_rotation.value())
        /** gyroscopes in rad/s, KVH puts out the integrated delta rotation */
        imusamples.gyro = imusamples.gyro * _sampling_frequency.value();

    /** Estimate the current timestamp */
    imusamples.time = timestamp_estimator.update(imusamples.time, kvh_driver->getCounter());

    /** Output information **/
    _raw_sensors.write(imusamples);
    _calibrated_sensors.write(imusamples);
    _device_temperature.write(base::Temperature::fromCelsius(boost::numeric_cast<double>(kvh_driver->getTemperature())));
    _timestamp_estimator_status.write(timestamp_estimator.getStatus());
}

void Task::updateHook()
{
    TaskBase::updateHook();
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
    kvh_driver.reset();
}

