/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <imu_kvh_1750/Driver.hpp>
#include <base/samples/IMUSensors.hpp>
#include <base/logging.h>

#ifndef D2R
#define D2R M_PI/180.00 /** Convert degree to radian **/
#endif
#ifndef R2D
#define R2D 180.00/M_PI /** Convert radian to degree **/
#endif

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
    kvh_driver->setReadTimeout(base::Time::fromMilliseconds(_timeout));
    kvh_driver->open(_device);
    fd = kvh_driver->getFileDescriptor();

    /*************************************/
    /** Configuration of Time estimator **/
    /*************************************/
    timestamp_estimator = new aggregator::TimestampEstimator(
	base::Time::fromSeconds(20),
	base::Time::fromSeconds(1.0 / imu_kvh_1750::DEFAULT_SAMPLING_FREQUENCY),
	base::Time::fromSeconds(0),
	INT_MAX);

    /******************************************/
    /** Configuration of the attitude filter **/
    /******************************************/
    Eigen::Matrix< double, IKFSTATEVECTORSIZE , 1  > x_0; /** Initial vector state **/
    Eigen::Matrix3d Ra; /** Measurement noise covariance matrix for acc */
    Eigen::Matrix3d Rg; /** Measurement noise covariance matrix for gyros */
    Eigen::Matrix3d Rm; /** Measurement noise covariance matrix for mag */
    Eigen::Matrix3d Ri; /** Measurement noise covariance matrix for inclinometers */
    Eigen::Matrix <double, IKFSTATEVECTORSIZE, IKFSTATEVECTORSIZE> P_0; /** Initial covariance matrix **/
    Eigen::Matrix3d Qbg; /** Noise for the gyros bias instability **/
    Eigen::Matrix3d Qba; /** Noise for the acc bias instability **/
    Eigen::Matrix3d Qbi; /** Noise for the inclinometers bias instability **/
    double sqrtdelta_t;

    /************************/
    /** Read configuration **/
    /************************/
    config = _filter_configuration.value();
    inertialnoise = _inertial_noise.value();
    adaptiveconfigAcc = _adaptive_config_acc.value();
    adaptiveconfigInc = _adaptive_config_inc.value();
    location = _location.value();

    /*************************/
    /** Noise configuration **/
    /*************************/
    sqrtdelta_t = sqrt(1.0/inertialnoise.bandwidth);
    //sqrtdelta_t = sqrt(_inertial_samples_period.value());

    Ra = Eigen::Matrix3d::Zero();
    Ra(0,0) = inertialnoise.accresolut[0] + pow(inertialnoise.accrw[0]/sqrtdelta_t,2);
    Ra(1,1) = inertialnoise.accresolut[1] + pow(inertialnoise.accrw[1]/sqrtdelta_t,2);
    Ra(2,2) = inertialnoise.accresolut[2] + pow(inertialnoise.accrw[2]/sqrtdelta_t,2);

    Rg = Eigen::Matrix3d::Zero();
    Rg(0,0) = pow(inertialnoise.gyrorw[0]/sqrtdelta_t,2);
    Rg(1,1) = pow(inertialnoise.gyrorw[1]/sqrtdelta_t,2);
    Rg(2,2) = pow(inertialnoise.gyrorw[2]/sqrtdelta_t,2);

    Rm = Eigen::Matrix3d::Zero();
    Rm(0,0) = pow(inertialnoise.magrw[0]/sqrtdelta_t,2);
    Rm(1,1) = pow(inertialnoise.magrw[1]/sqrtdelta_t,2);
    Rm(2,2) = pow(inertialnoise.magrw[2]/sqrtdelta_t,2);

    Ri = Eigen::Matrix3d::Zero();
    Ri(0,0) = inertialnoise.incresolut[0] + pow(inertialnoise.incrw[0]/sqrtdelta_t,2);
    Ri(1,1) = inertialnoise.incresolut[1] + pow(inertialnoise.incrw[1]/sqrtdelta_t,2);
    Ri(2,2) = inertialnoise.incresolut[2] + pow(inertialnoise.incrw[2]/sqrtdelta_t,2);

    /** Noise for error in gyros bias instability **/
    //TODO use asDiagonal
    Qbg.setZero();
    Qbg(0,0) = pow(inertialnoise.gbiasins[0],2);
    Qbg(1,1) = pow(inertialnoise.gbiasins[1],2);
    Qbg(2,2) = pow(inertialnoise.gbiasins[2],2);

    /** Noise for error in accelerometers bias instability **/
    Qba.setZero();
    Qba(0,0) = pow(inertialnoise.abiasins[0],2);
    Qba(1,1) = pow(inertialnoise.abiasins[1],2);
    Qba(2,2) = pow(inertialnoise.abiasins[2],2);

    /** Noise for error in inclinometers bias instability **/
    Qbi.setZero();
    Qbi(0,0) = pow(inertialnoise.ibiasins[0],2);
    Qbi(1,1) = pow(inertialnoise.ibiasins[1],2);
    Qbi(2,2) = pow(inertialnoise.ibiasins[2],2);


    /** Initial error covariance **/
    P_0 = Eigen::Matrix <double,IKFSTATEVECTORSIZE,IKFSTATEVECTORSIZE>::Zero();
    P_0.block <3, 3> (0,0) = 1.0e-06 * Eigen::Matrix3d::Identity();//Error quaternion
    P_0.block <3, 3> (3,3) = 1.0e-06 * Eigen::Matrix3d::Identity();//Gyros bias
    P_0.block <3, 3> (6,6) = 1.0e-06 * Eigen::Matrix3d::Identity();//Accelerometers bias
    //TODO following may fail for ikfstatevectorsize without inclinometers...
    //P_0.block <3, 3> (9,9) = 1.0e-06 * Eigen::Matrix3d::Identity();//Inclinometers bias

    /** Theoretical Gravity **/
    double gravity = GRAVITY;
    if (location.latitude > 0 && location.latitude < 90){
        gravity = GravityModel (location.latitude, location.altitude);
    }

    /** Initialize the filter, including the adaptive part **/
    //TODO adapt to using only acc, gyr
    myfilter.Init(P_0, Ra, Rg, Rm, Ri, Qbg, Qba, Qbi, gravity, location.dip_angle,
            adaptiveconfigAcc.M1, adaptiveconfigAcc.M2, adaptiveconfigAcc.gamma,
            adaptiveconfigInc.M1, adaptiveconfigInc.M2, adaptiveconfigInc.gamma);

    /** Leveling configuration **/
    init_leveling_samples.resize(3, config.init_leveling_samples);

    /** Set the index to Zero **/
    init_leveling_idx = 0;

    /** Oldomega initial **/
    oldomega.setZero();

    initAttitude = false;

    /** Output variable **/
    orientationOut.invalidate();
    orientationOut.sourceFrame = config.source_frame_name;
    orientationOut.targetFrame = config.target_frame_name;

    #ifdef DEBUG_PRINTS
    std::cout<< "Rg\n"<<Rg<<"\n";
    std::cout<< "Ra\n"<<Ra<<"\n";
    std::cout<< "Rm\n"<<Rm<<"\n";
    std::cout<< "Ri\n"<<Ri<<"\n";
    std::cout<< "P_0\n"<<P_0<<"\n";
    std::cout<< "Qbg\n"<<Qbg<<"\n";
    std::cout<< "Qba\n"<<Qba<<"\n";
    std::cout<< "Qbi\n"<<Qbi<<"\n";
    #endif

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
        //TODO check activity timeout
	activity->setTimeout(2*_timeout);
    }

    return true;
}


void Task::updateHook()
{
  //std::cout << "UpdateHook called" << std::endl;
  base::samples::IMUSensors imusamples;
  TaskBase::updateHook();

  RTT::extras::FileDescriptorActivity* act = getActivity<RTT::extras::FileDescriptorActivity>();
  if(act->hasError()) LOG_ERROR("FD Activity has error");
  if(act->hasTimeout()) LOG_WARN("FD Activity timeout");
  if(act->isUpdated(fd)){
    kvh_driver->read();
    imusamples = kvh_driver->getIMUReading();


    /** Time is current time minus the latency **/
    //base::Time recvts = base::Time::now() - base::Time::fromMicroseconds(stim300_driver->getPacketLatency());
    base::Time recvts = base::Time::now();//no latency estimation from KVH sensor available

    int packet_counter = kvh_driver->getCounter();
    base::Time ts = timestamp_estimator->update(recvts,packet_counter);
    base::Time diffTime = ts - prev_ts;

    imusamples.time = ts;
    _raw_sensors.write(imusamples);

    prev_ts = ts;
#ifdef DEBUG_PRINTS
//    std::cout<<"Delta time[s]: "<<diffTime.toSeconds()<<"\n";
#endif

    if (_use_filter.value())
    {
      /** Attitude filter **/
      if (!initAttitude)
      {
#ifdef DEBUG_PRINTS
        std::cout<<"** [ORIENT_IKF] Initial Attitude["<<init_leveling_idx<<"]\n";
#endif

        if (config.use_inclinometers)
          init_leveling_samples.col(init_leveling_idx) = imusamples.mag;
        else
          init_leveling_samples.col(init_leveling_idx) = imusamples.acc;

        init_leveling_idx++;

        if (init_leveling_idx >= config.init_leveling_samples)
        {
          Eigen::Matrix <double,3,1> meansamples, euler;

          meansamples[0] = init_leveling_samples.row(0).mean();
          meansamples[1] = init_leveling_samples.row(1).mean();
          meansamples[2] = init_leveling_samples.row(2).mean();

          if ((config.init_leveling_samples > 0) && (base::isnotnan(meansamples)) &&(meansamples.norm() < (GRAVITY+GRAVITY_MARGING)))
          {
            euler[0] = (double) asin((double)meansamples[1]/ (double)meansamples.norm()); // Roll
            euler[1] = (double) -atan(meansamples[0]/meansamples[2]); //Pitch
            euler[2] = M_PI;//Yaw (Work around for STIM3000 in Asguard)

            /** Set the initial attitude  **/
            attitude = Eigen::Quaternion <double> (Eigen::AngleAxisd(euler[2], Eigen::Vector3d::UnitZ())*
                Eigen::AngleAxisd(euler[1], Eigen::Vector3d::UnitY()) *
                Eigen::AngleAxisd(euler[0], Eigen::Vector3d::UnitX()));

            if (config.use_samples_as_theoretical_gravity)
              myfilter.setGravity(meansamples.norm());
          }
          else
          {
            attitude.setIdentity();
          }

          myfilter.setAttitude(attitude);
          initAttitude = true;

#ifdef DEBUG_PRINTS
          std::cout<< "******** Initial Attitude  *******"<<"\n";
          std::cout<< "Init Roll: "<<euler[0]*R2D<<" Init Pitch: "<<euler[1]*R2D<<" Init Yaw: "<<euler[2]*R2D<<"\n";
#endif
        }
      }
      else
      {
        double delta_t = diffTime.toSeconds();
        Eigen::Vector3d acc, gyro, incl;
        //TODO inclinometer reading set to imusamples.mag???
        acc = imusamples.acc; gyro = imusamples.gyro; incl = imusamples.mag;

        /** Eliminate Earth rotation **/
        if (config.init_leveling_samples > 0)
        {
          Eigen::Quaterniond q_body2world = myfilter.getAttitude().inverse();
          SubtractEarthRotation(gyro, q_body2world, location.latitude);
          imusamples.gyro = gyro;
        }

        /** Predict **/
        myfilter.predict(gyro, delta_t);

        /** Update/Correction **/
        myfilter.update(acc, true, incl, config.use_inclinometers);

        /** Delta quaternion of this step **/
        deltaquat = attitude.inverse() * myfilter.getAttitude();

        /** Delta quaternion of this step **/
        deltahead = deltaHeading(gyro, oldomega, delta_t);

      }
    }


    /** Output information **/
    this->outputPortSamples(kvh_driver, myfilter, imusamples);

    _calibrated_sensors.write(imusamples);
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

Eigen::Quaternion<double> Task::deltaHeading(const Eigen::Vector3d &angvelo, Eigen::Matrix4d &oldomega, const double delta_t)
{
    Eigen::Matrix4d omega;
    Eigen::Quaternion<double> deltahead;

    omega << 0,-angvelo(0), -angvelo(1), -angvelo(2),
		angvelo(0), 0, angvelo(2), -angvelo(1),
		angvelo(1), -angvelo(2), 0, angvelo(0),
		angvelo(2), angvelo(1), -angvelo(0), 0;


    deltahead = deltaQuaternion(angvelo, oldomega, omega, delta_t);

    oldomega = omega;

    return deltahead;
}

void Task::outputPortSamples(imu_kvh_1750::Driver *driver, filter::Ikf<double, true, false> &myfilter, const base::samples::IMUSensors &imusamples)
{
    Eigen::Matrix <double,IKFSTATEVECTORSIZE,IKFSTATEVECTORSIZE> Pk = myfilter.getCovariance();
    base::samples::IMUSensors compensatedSamples;

    if (_use_filter.value())
    {
        /** Merge the two delta quaternion **/
        Eigen::AngleAxisd headangle(deltahead);
        Eigen::AngleAxisd deltaangle(deltaquat);
        Eigen::Vector3d scaleangle = deltaangle.angle() * deltaangle.axis();
        scaleangle[2] = (headangle.angle() * headangle.axis())[2];

        /** Update globally attitude **/
        attitude = attitude * Eigen::Quaternion <double> (Eigen::AngleAxisd(scaleangle[2], Eigen::Vector3d::UnitZ())*
                Eigen::AngleAxisd(scaleangle[1], Eigen::Vector3d::UnitY()) *
                Eigen::AngleAxisd(scaleangle[0], Eigen::Vector3d::UnitX()));

        orientationOut.time = imusamples.time;
        orientationOut.orientation = myfilter.getAttitude();
       // orientationOut.orientation = attitude;
        orientationOut.cov_orientation = Pk.block<3,3>(0,0);
        _orientation_samples_out.write(orientationOut);

        compensatedSamples = imusamples;
        compensatedSamples.gyro = imusamples.gyro - myfilter.getGyroBias();//gyros minus bias
        compensatedSamples.acc = imusamples.acc - myfilter.getAccBias() - myfilter.getGravityinBody(); //acc minus bias and gravity
        _calibrated_sensors.write(compensatedSamples);

        #ifdef DEBUG_PRINTS
        Eigen::Vector3d euler;
        euler[2] = orientationOut.orientation.toRotationMatrix().eulerAngles(2,1,0)[0];//Yaw
        euler[1] = orientationOut.orientation.toRotationMatrix().eulerAngles(2,1,0)[1];//Pitch
        euler[0] = orientationOut.orientation.toRotationMatrix().eulerAngles(2,1,0)[2];//Roll
        std::cout<< "Roll: "<<euler[0]*R2D<<" Pitch: "<<euler[1]*R2D<<" Yaw: "<<euler[2]*R2D<<"\n";
        //Eigen::AngleAxisd angleaxis(orientationOut.orientation);
        //euler = angleaxis.angle() * angleaxis.axis();
        //std::cout<< "Roll: "<<euler[0]*R2D<<" Pitch: "<<euler[1]*R2D<<" Yaw: "<<euler[2]*R2D<<"\n";
        #endif
    }
    else
    {
        _calibrated_sensors.write(imusamples);
    }

}
