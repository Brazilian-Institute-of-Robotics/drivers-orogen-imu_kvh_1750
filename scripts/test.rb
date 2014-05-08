require 'orocos'
#require 'vizkit'
include Orocos

#if !ARGV[0]
#    STDERR.puts "usage: test.rb <device name>"
#    exit 1
#end

Orocos.initialize

Orocos.run 'imu_kvh_1750::Task' => 'imu' do
#    Orocos.log_all_ports

    Orocos.conf.load_dir('../config')
    imu = Orocos.name_service.get 'imu'

    Orocos.conf.apply(imu, ['default','Bremen','imu_kvh_1750'], :override => true)


#    imu.device = ARGV[0]
    imu.device = "serial:///dev/ttyUSB0:921600"
    imu.timeout = 20 
    imu.use_filter = true
#    imu.use_filter = false
    imu.initial_heading = 0.143

    imu.configure
    imu.start
    puts "done."

    Orocos.watch(imu)
end

