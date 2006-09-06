require "ruby_bluez"

a = Bluetooth::Devices.scan
a.each { |device|
  puts device.addr
  puts device.name
  p device
}
p a
