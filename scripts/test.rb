require 'orocos'
#require 'vizkit'
include Orocos

if !ARGV[0]
    STDERR.puts "usage: test.rb <device name>"
    exit 1
end

#ENV['PKG_CONFIG_PATH'] = "#{File.expand_path("..", File.dirname(__FILE__))}/build:#{ENV['PKG_CONFIG_PATH']}"

Orocos.initialize

Orocos.run 'imu_kvh_1750::Task' => 'imu' do
    Orocos.log_all_ports

    fog = Orocos.name_service.get 'imu'

    fog.device = ARGV[0]

    fog.configure
    fog.start
    puts "done."

    Orocos.watch(fog)
end

