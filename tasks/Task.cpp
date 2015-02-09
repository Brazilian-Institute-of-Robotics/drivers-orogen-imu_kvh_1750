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
      timestamp_estimator(NULL),
      fd(0)
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

    kvh_driver = new imu_kvh_1750::Driver();
    kvh_driver->setReadTimeout(base::Time::fromMilliseconds(_timeout.value()));
    kvh_driver->open(_device.value());
    fd = kvh_driver->getFileDescriptor();
    
    if(fd < 0)
    {
	RTT::log(RTT::Error) << "Failed to open device. No valid file descriptor." << RTT::endlog();
    }

    /*************************************/
    /** Configuration of Time estimator **/
    /*************************************/
    timestamp_estimator = new aggregator::TimestampEstimator(
	base::Time::fromSeconds(20),
	base::Time::fromSeconds(1.0 / imu_kvh_1750::DEFAULT_SAMPLING_FREQUENCY),
	base::Time::fromSeconds(0),
	INT_MAX);
    
    /** Task states **/
    last_state = PRE_OPERATIONAL;
    new_state = RUNNING;

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
	activity->setTimeout(boost::numeric_cast<int>((2.0*_timeout.value())));
    }

    return true;
}


void Task::updateHook()
{
    new_state = RUNNING;
    TaskBase::updateHook();

    RTT::extras::FileDescriptorActivity* act = getActivity<RTT::extras::FileDescriptorActivity>();
    if(act->hasError()) 
	RTT::log(RTT::Warning) << "File descriptor activity has an error." << RTT::endlog();
    if(act->hasTimeout()) 
	RTT::log(RTT::Warning) << "File descriptor activity timeout." << RTT::endlog();
    if(act->isUpdated(fd))
    {
	base::samples::IMUSensors imusamples;
	kvh_driver->read();
	imusamples = kvh_driver->getIMUReading();
	imusamples.acc = imusamples.acc * GRAVITY_SI; //g to m/s^2, KVH puts out acceleration in g

	/** Estimate the current timestamp */
	imusamples.time = timestamp_estimator->update(imusamples.time, kvh_driver->getCounter());

	/** Output information **/
	_raw_sensors.write(imusamples);

	_calibrated_sensors.write(imusamples);
	
	/** Write tast state if it has changed **/
	if(last_state != new_state)
	{
	    last_state = new_state;
	    state(new_state);
	}

	_timestamp_estimator_status.write(timestamp_estimator->getStatus());
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
    delete(timestamp_estimator);
}