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

    Orocos.conf.apply(imu, ['default'], :override => true)


#    imu.device = ARGV[0]
    imu.device = "serial:///dev/ttyS2:921600"
    imu.timeout = 20 

    imu.configure
    imu.start
    puts "done."

    Orocos.watch(imu)
end

