// Include the Ruby headers and goodies
#include <ruby.h>
#include <rubyio.h>
#include <rubysig.h>
#include <util.h>
#include "bluetooth_linux.h"

VALUE bt_module;
VALUE bt_device_class;
VALUE bt_devices_class;
VALUE bt_socket_class;
VALUE bt_rfcomm_socket_class;
VALUE bt_l2cap_socket_class;
VALUE bt_service_class;

// The initialization method for this module
void Init_ruby_bluetooth()
{
  bt_module = rb_define_module("Bluetooth");
  bt_device_class = rb_define_class_under(bt_module, "Device", rb_cObject);
  bt_devices_class = rb_define_class_under(bt_module, "Devices", rb_cObject);

  bt_socket_class = rb_define_class_under(bt_module, "BluetoothSocket", rb_cIO);
  rb_define_method(bt_socket_class, "inspect", bt_socket_inspect, 0);
  rb_define_method(bt_socket_class, "for_fd", bt_socket_s_for_fd, 1);
  rb_undef_method(bt_socket_class, "initialize");

  bt_rfcomm_socket_class = rb_define_class_under(bt_module, "RFCOMMSocket", bt_socket_class);
  rb_define_method(bt_rfcomm_socket_class, "initialize", bt_rfcomm_socket_init, -1);
  rb_define_method(bt_rfcomm_socket_class, "connect", bt_rfcomm_socket_connect, 2);

  bt_l2cap_socket_class = rb_define_class_under(bt_module, "L2CAPSocket", bt_socket_class);
  rb_define_method(bt_l2cap_socket_class, "initialize", bt_l2cap_socket_init, -1);
  rb_define_method(bt_l2cap_socket_class, "connect", bt_l2cap_socket_connect, 2);

  rb_define_singleton_method(bt_devices_class, "scan", bt_devices_scan, 0);
  rb_define_singleton_method(bt_device_class, "new", bt_device_new, 2);
  rb_define_attr(bt_device_class, "addr", Qtrue, Qfalse);
  rb_define_attr(bt_device_class, "name", Qtrue, Qfalse);
  
  bt_service_class = rb_define_class_under(bt_module, "Service", rb_cObject);
  rb_define_singleton_method(bt_service_class, "new", bt_service_new, 3);
  rb_define_attr(bt_device_class, "name", Qtrue, Qfalse);
  rb_define_attr(bt_device_class, "description", Qtrue, Qfalse); 
  rb_define_attr(bt_device_class, "provider", Qtrue, Qfalse); 
}

static VALUE bt_service_new(VALUE self, VALUE name, VALUE description, VALUE provider) {
  struct bluetooth_service_struct *bss;

  VALUE obj = Data_Make_Struct(self,
                               struct bluetooth_service_struct, NULL,
                               free, bss);

  rb_iv_set(obj, "@name", name);
  rb_iv_set(obj, "@description", description);
  rb_iv_set(obj, "@provider", provider);

  return obj;
}

static VALUE
bt_l2cap_socket_connect(VALUE self, VALUE host, VALUE port)
{
    OpenFile *fptr;
	int fd;

	GetOpenFile(self, fptr);
	fd = fileno(fptr->f);

    struct sockaddr_l2 addr = { 0 };
    char *dest = STR2CSTR(host);

    // set the connection parameters (who to connect to)
    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = (uint8_t) FIX2UINT(port);
    str2ba( dest, &addr.l2_bdaddr );

    // connect to server
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      rb_sys_fail("connect(2)");
	}

	return INT2FIX(0);
}

static VALUE
bt_rfcomm_socket_connect(VALUE self, VALUE host, VALUE port)
{
    OpenFile *fptr;
	int fd;

	GetOpenFile(self, fptr);
	fd = fileno(fptr->f);

    struct sockaddr_rc addr = { 0 };
    char *dest = STR2CSTR(host);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) FIX2UINT(port);
    str2ba( dest, &addr.rc_bdaddr );

    // connect to server
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      rb_sys_fail("connect(2)");
	}

	return INT2FIX(0);
}

static VALUE
bt_socket_s_for_fd(VALUE klass, VALUE fd)
{
  OpenFile *fptr;
  VALUE sock = bt_init_sock(rb_obj_alloc(klass), NUM2INT(fd));

  GetOpenFile(sock, fptr);
  return sock;
}

static VALUE bt_socket_inspect(VALUE self)
{
  return self;
}

static int
bt_ruby_socket(int domain, int type, int proto)
{
    int fd;

    fd = socket(domain, type, proto);
    if (fd < 0) {
  if (errno == EMFILE || errno == ENFILE) {
      rb_gc();
      fd = socket(domain, type, proto);
  }
    }
    return fd;
}

static VALUE
bt_init_sock(VALUE sock, int fd)
{
    OpenFile *fp = NULL;

    MakeOpenFile(sock, fp);

    fp->f = rb_fdopen(fd, "r");
    fp->f2 = rb_fdopen(fd, "w");
    fp->mode = FMODE_READWRITE;

    rb_io_synchronized(fp);

    return sock;
}

// Initialization of a RFCOMM socket
static VALUE bt_rfcomm_socket_init(int argc, VALUE *argv, VALUE sock)
{
  int fd = bt_ruby_socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (fd < 0) {
    rb_sys_fail("socket(2) - bt");
  }
  VALUE ret = bt_init_sock(sock, fd);
  return ret;
}

// Initialization of a L2CAP socket
static VALUE bt_l2cap_socket_init(int argc, VALUE *argv, VALUE sock)
{
  int fd = bt_ruby_socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
  if (fd < 0) {
    rb_sys_fail("socket(2) - bt");
  }
  VALUE ret = bt_init_sock(sock, fd);
  return ret;
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
  inquiry_info *ii = NULL;
  int max_rsp, num_rsp;
  int dev_id, sock, len, flags;
  int i;

  dev_id = hci_get_route(NULL);
  sock = hci_open_dev( dev_id );
  if (dev_id < 0 || sock < 0)
    {
      rb_raise (rb_eIOError, "error opening socket");
    }

  len  = 8;
  max_rsp = 255;
  flags = IREQ_CACHE_FLUSH;
  ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

  num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
  if( num_rsp < 0 )
    rb_raise(rb_eIOError, "hci_inquiry");

  VALUE devices_array = rb_ary_new();

  // Iterate over every device found and add it to result array
  for (i = 0; i < num_rsp; i++)
    {
      char addr[19] = { 0 };
      char name[248] = { 0 };

      ba2str(&(ii+i)->bdaddr, addr);
      memset(name, 0, sizeof(name));
      if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name),
                               name, 0) < 0)
        strcpy(name, "[unknown]");

      VALUE bt_dev = bt_device_new(bt_device_class,
                                  rb_str_new2(name), rb_str_new2(addr));

      rb_ary_push(devices_array, bt_dev);
    }

  free( ii );
  close( sock );
  return devices_array;
}
