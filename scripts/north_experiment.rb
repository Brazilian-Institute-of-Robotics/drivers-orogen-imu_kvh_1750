require 'orocos'
include Orocos

Orocos.initialize

Orocos.run 'imu_kvh_1750::Task' => 'imu' do
    Orocos.log_all_ports

    Orocos.conf.load_dir('../config')
    imu = Orocos.name_service.get 'imu'

    Orocos.conf.apply(imu, ['default','Bremen','imu_kvh_1750'], :override => true)

    imu.device = "serial:///dev/ttyUSB0:921600"
    imu.timeout = 20 
    imu.use_filter = true
    imu.initial_north_seeking = false #default false
    imu.north_seeking_period = 300 #default 90

    imu.configure
    imu.start
    
    Orocos.watch(imu)
end
