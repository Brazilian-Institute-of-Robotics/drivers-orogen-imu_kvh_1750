require 'orocos'
#require 'vizkit'
include Orocos

if !ARGV[0]
    STDERR.puts "usage: test.rb <device name>"
    exit 1
end

Orocos.initialize

Orocos.run 'imu_kvh_1750::Task' => 'imu' do
#    Orocos.log_all_ports

    imu = Orocos.name_service.get 'imu'

    imu.device = ARGV[0]

    imu.configure
    imu.start
    puts "done."

    Orocos.watch(imu)
end

