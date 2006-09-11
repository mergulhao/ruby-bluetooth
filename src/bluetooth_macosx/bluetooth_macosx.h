#import <unistd.h>

#import <Cocoa/Cocoa.h>

#import <IOBluetooth/IOBluetoothUserLib.h>
#import <IOBluetooth/objc/IOBluetoothDevice.h>

// Prototype for the initialization method - Ruby calls this, not you
void Init_ruby_bluetooth();

struct bluetooth_device_struct
  {
    VALUE addr;
    VALUE name;
  };

static VALUE bt_device_new(VALUE self, VALUE name, VALUE addr);

static VALUE bt_devices_scan(VALUE self);

//===========================================================================================================================
//// Forwards
////===========================================================================================================================
//
@class IOBluetoothDeviceInquiry;

@interface BluetoothDeviceScanner : NSObject
{
    IOBluetoothDeviceInquiry *      _inquiry;
    BOOL                            _busy;

	VALUE _foundDevices;
}

-(void)stopSearch;
-(IOReturn)startSearch;
-(BOOL) isBusy;
@end
