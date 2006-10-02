#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

// Prototype for the initialization method - Ruby calls this, not you
void Init_ruby_bluetooth();

struct bluetooth_device_struct
  {
    VALUE addr;
    VALUE name;
  };

struct bluetooth_service_struct
  {
    VALUE name;
    VALUE description;
    VALUE provider;
  };

static VALUE bt_device_new(VALUE self, VALUE name, VALUE addr);

static VALUE bt_devices_scan(VALUE self);

static int bt_ruby_socket(int domain, int type, int proto);

static VALUE bt_init_sock(VALUE sock, int fd);

static VALUE bt_socket_inspect(VALUE self);

static VALUE bt_socket_s_for_fd(VALUE klass, VALUE fd);

static VALUE bt_rfcomm_socket_init(int argc, VALUE *argv, VALUE sock);

static VALUE bt_rfcomm_socket_connect(VALUE sock, VALUE host, VALUE port);

static VALUE bt_l2cap_socket_init(int argc, VALUE *argv, VALUE sock);

static VALUE bt_l2cap_socket_connect(VALUE sock, VALUE host, VALUE port);

static VALUE bt_service_new(VALUE self, VALUE name, VALUE description, VALUE provider);
