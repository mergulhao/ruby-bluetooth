// Prototype for the initialization method - Ruby calls this, not you
void Init_ruby_bluetooth();

struct bluetooth_device_struct
  {
    VALUE addr;
    VALUE name;
  };

static VALUE bt_device_new(VALUE self, VALUE name, VALUE addr);

static VALUE bt_devices_scan(VALUE self);
