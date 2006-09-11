// Include the Ruby headers and goodies
#import <ruby.h>
#import <rubyio.h>
#import <rubysig.h>
#import <util.h>
#import <unistd.h>

#import <Foundation/NSDictionary.h>

#import <IOBluetooth/objc/IOBluetoothDeviceInquiry.h>
#import <IOBluetooth/IOBluetoothUserLib.h>

#import "bluetooth_macosx.h"

VALUE bt_module;
VALUE bt_device_class;
VALUE bt_devices_class;
BOOL BUSY = false;

// The initialization method for this module
void Init_ruby_bluetooth()
{
    bt_module = rb_define_module("Bluetooth");
    bt_device_class = rb_define_class_under(bt_module, "Device", rb_cObject);
    bt_devices_class = rb_define_class_under(bt_module, "Devices", rb_cObject);
    rb_define_singleton_method(bt_devices_class, "scan", bt_devices_scan, 0);
    rb_define_singleton_method(bt_device_class, "new", bt_device_new, 2);
    rb_define_attr(bt_device_class, "addr", Qtrue, Qfalse);
    rb_define_attr(bt_device_class, "name", Qtrue, Qfalse);
    rb_undef_method(bt_devices_class, "initialize");
}

// Create a Device, right now it only holds name and addr
static VALUE bt_device_new(VALUE self, VALUE name, VALUE addr)
{
    struct bluetooth_device_struct *bds;

    VALUE obj = Data_Make_Struct(self,
                                 struct bluetooth_device_struct, NULL,
                                 free, bds);

    rb_iv_set(obj, "@name", name);
    rb_iv_set(obj, "@addr", addr);

    return obj;
}

// Scan local network for visible remote devices
static VALUE bt_devices_scan(VALUE self)
{
    BluetoothDeviceScanner *bds = [BluetoothDeviceScanner new];
    [bds startSearch];
	while([bds isBusy]);
    return [bds foundDevices];
}

@implementation BluetoothDeviceScanner

//===========================================================================================================================
// deviceInquiryDeviceFound
//===========================================================================================================================

- (void)    deviceInquiryDeviceFound:(IOBluetoothDeviceInquiry*)sender  device:(IOBluetoothDevice*)device
{
    const BluetoothDeviceAddress* addressPtr = [device getAddress];
    deviceAddressString = [NSString stringWithFormat:@"[%02x:%02x:%02x:%02x:%02x:%02x]",
	  addressPtr->data[0],
      addressPtr->data[1],
      addressPtr->data[2],
      addressPtr->data[3],
      addressPtr->data[4],
      addressPtr->data[5]];

	NSString* deviceNameString = [inDevice getName];
    if( !deviceNameString ) {
      deviceNameString = @"[unknown]";
    }

    VALUE bt_dev = bt_device_new(bt_device_class, rb_str_new2([deviceNameString UTF8String]),
	  rb_str_new2([deviceAddressString UTF8String]));
    rb_ary_push(_foundDevices, bt_dev);
}

-(IOReturn)startSearch
{
    IOReturn    status;

    [self   stopSearch];

    _inquiry = [IOBluetoothDeviceInquiry    inquiryWithDelegate:self];

	// Should clear array, not simply create a new one
	_foundDevices = rb_ary_new();

    status = [_inquiry  start];
    if( status == kIOReturnSuccess )
    {
    [_inquiry   retain];

        _busy = TRUE;
    }
    return( status );
}

//===========================================================================================================================
// stopSearch
//===========================================================================================================================

-(void)stopSearch
{
    if( _inquiry )
    {
    [_inquiry stop];
        [_inquiry release];
        _inquiry = nil;
    }
}
//===========================================================================================================================
// deviceInquiryComplete
//===========================================================================================================================

- (void)    deviceInquiryComplete:(IOBluetoothDeviceInquiry*)sender error:(IOReturn)error   aborted:(BOOL)aborted
{
  _busy = FALSE;
}

- (BOOL) isBusy
{
  return _busy;
}
@end
