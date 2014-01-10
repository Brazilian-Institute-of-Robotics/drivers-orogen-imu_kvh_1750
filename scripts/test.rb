require 'orocos'
require 'vizkit'
include Orocos

if !ARGV[0]
    STDERR.puts "usage: test.rb <device name>"
    exit 1
end

ENV['PKG_CONFIG_PATH'] = "#{File.expand_path("..", File.dirname(__FILE__))}/build:#{ENV['PKG_CONFIG_PATH']}"

Orocos.initialize

Orocos.run 'orogen_default_fog_kvh_1750__Task' do
    Orocos.log_all_ports

    fog = Orocos.name_service.get 'orogen_default_fog_kvh_1750__Task'

    fog.device = ARGV[0]

    fog.configure
    fog.start
    puts "done."

#    Vizkit::UiLoader.register_default_widget_for "RangeView", "/base/samples/LaserScan"
#    Vizkit.display hokuyo.scans
#    Vizkit.exec

    loop do
       sleep 0.1
    end
    
    fog.stop
end

