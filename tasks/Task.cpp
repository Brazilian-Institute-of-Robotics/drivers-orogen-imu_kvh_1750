/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <imu_kvh_1750/Driver.hpp>
#include <base/samples/IMUSensors.hpp>
#include <base/logging.h>
#include <Eigen/Geometry>
#include <boost/numeric/conversion/cast.hpp>

using namespace imu_kvh_1750;

Task::Task(std::string const& name)
    : TaskBase(name),
      fd(0),
      roll_old(0)
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
    if (! TaskBase::configureHook())
        return false;
    /************************************/
    /** Configure IMU KVH 1750 driver  **/
    /************************************/

    kvh_driver.reset(new imu_kvh_1750::Driver());
    kvh_driver->setReadTimeout(base::Time::fromSeconds(_timeout.value()));
    kvh_driver->open(_device.value());
    fd = kvh_driver->getFileDescriptor();

    if(_axes_orientation_acc.value().isZero())
    {
        LOG_ERROR("The accelerometers' transformation matrix was not set.");
    }

    if(_axes_orientation_gyro.value().isZero())
    {
        LOG_ERROR("The gyroscopes' transformation matrix was not set.");
    }

    if(fd < 0)
    {
	RTT::log(RTT::Error) << "Failed to open device. No valid file descriptor." << RTT::endlog();
	return false;
    }

    /*************************************/
    /** Configuration of Time estimator **/
    /*************************************/
    if(_sampling_frequency.value() <= 0.0)
    {
	RTT::log(RTT::Error) << "The sampling frequency has to be a positive non-zero value." << RTT::endlog();
	return false;
    }
    
    timestamp_estimator.reset(new aggregator::TimestampEstimator(
	base::Time::fromSeconds(20),
	base::Time::fromSeconds(1.0 / _sampling_frequency.value()),
	base::Time::fromSeconds(0),
	INT_MAX));

    return true;
}

bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;

    RTT::extras::FileDescriptorActivity* activity =
        getActivity<RTT::extras::FileDescriptorActivity>();
    if (activity)
    {
        activity->watch(fd);
	base::Time activity_timeout = base::Time::fromSeconds(2.0*_timeout.value());
	activity->setTimeout(boost::numeric_cast<int>(activity_timeout.toMilliseconds()));
    }

    return true;
}


void Task::updateHook()
{
    TaskBase::updateHook();

    RTT::extras::FileDescriptorActivity* act = getActivity<RTT::extras::FileDescriptorActivity>();
    if (act->hasError())
        RTT::log(RTT::Warning) << "File descriptor activity has an error." << RTT::endlog();
    if (act->hasTimeout())
        RTT::log(RTT::Warning) << "File descriptor activity timeout." << RTT::endlog();
    if (act->isUpdated(fd)) {
        base::samples::IMUSensors imusamples;
        kvh_driver->read();
        imusamples = kvh_driver->getIMUReading();

        /** rotate measurments to the local frame */
        imusamples.acc = _axes_orientation_acc.value() * imusamples.acc;
        imusamples.gyro = _axes_orientation_gyro.value() * imusamples.gyro;

        /** acceleration in m/s^2 */
        imusamples.acc = imusamples.acc * GRAVITY_SI; //g to m/s^2, KVH puts out acceleration in g

        if (_gyroscope_delta_rotation.value())
            /** gyroscopes in rad/s, KVH puts out the integrated delta rotation */
            imusamples.gyro = imusamples.gyro * _sampling_frequency.value();

        /** Estimate the current timestamp */
        imusamples.time = timestamp_estimator->update(imusamples.time, kvh_driver->getCounter());


        double roll = (double) asin((double) imusamples.acc[1] / (double) imusamples.acc.norm()); // Roll

        if (!time_old.isNull()) {

            /** Estimate the roll using accelerometer */
            imusamples.gyro[0] = (roll - roll_old) / (imusamples.time - time_old).toSeconds();

            /** Output information **/
            _raw_sensors.write(imusamples);

            _calibrated_sensors.write(imusamples);
        }


        _device_temperature.write(base::Temperature::fromCelsius(boost::numeric_cast<double>(kvh_driver->getTemperature())));

        _timestamp_estimator_status.write(timestamp_estimator->getStatus());

        roll_old = roll;
        time_old = imusamples.time;
    }
}

void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    TaskBase::stopHook();

    RTT::extras::FileDescriptorActivity* activity =
        getActivity<RTT::extras::FileDescriptorActivity>();
    if (activity)
    {
        activity->clearAllWatches();
        //set timeout back so we don't timeout on the rtt's pipe
	activity->setTimeout(0);
    }

    timestamp_estimator->reset();
}

void Task::cleanupHook()
{
    TaskBase::cleanupHook();
    kvh_driver->close();
    fd = 0;
    timestamp_estimator.reset();
    kvh_driver.reset();
}
