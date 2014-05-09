require 'orocos'
#require 'vizkit'
include Orocos

#if !ARGV[0]
#    STDERR.puts "usage: test.rb <device name>"
#    exit 1
#end

Orocos.initialize

Orocos.run 'imu_kvh_1750::Task' => 'imu', 'north_seeker::Task' => 'seeker' do
#    Orocos.log_all_ports

    imu = Orocos.name_service.get 'imu'
    seeker = Orocos.name_service.get 'seeker'

    Orocos.conf.load_dir('../config')
    Orocos.conf.apply(imu, ['default','Bremen','imu_kvh_1750'], :override => true)

    imu.device = "serial:///dev/ttyUSB0:921600"
    imu.timeout = 20 
    imu.use_filter = false
    
    seeker.sampling_period = 30
    
    imu.configure
    seeker.configure
    
    imu.start
    seeker.start

    imu.raw_sensors.connect_to seeker.imusamples 
    
    seek_reader = seeker.heading.reader
    
    while true 
      heading = seek_reader.read_new
      if heading != nil 
        break
      end
    end

    puts "starting orientation estimation"
    seeker.stop
    imu.stop

    imu.initial_heading = heading
    imu.use_filter = true
    imu.configure
    imu.start
    
    Orocos.watch(imu)
end
