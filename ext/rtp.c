#include <ortp/ortp.h>
#include <ortp/rtp.h>
#include <ortp/rtpsession.h>
#include "ruby.h"

VALUE rb_mRTP;
VALUE rb_cRSession;
VALUE sym_mode, sym_remote, sym_local;
VALUE ip_regexp;

static void
session_free(ptr)
  RtpSession *ptr;
{
  rtp_session_destroy(ptr);
}

static VALUE
session_alloc(klass)
  VALUE klass;
{
  return Data_Wrap_Struct(klass, 0, session_free, 0);
}

static VALUE
session_init(argc, argv, self)
  int argc;
  VALUE *argv, self;
{
  VALUE options, opt_mode, opt_remote, opt_local;
  VALUE remote_addr, local_addr;
  RtpSession *session;
  int mode, len, len2, remote_port, local_port;
  char *str, *sptr;

  rb_scan_args(argc, argv, "01", &options);
  if (NIL_P(options)) {
    options = rb_hash_new();
  }

  // mode
  opt_mode = rb_hash_aref(options, sym_mode);
  if (NIL_P(opt_mode)) {
    mode = RTP_SESSION_SENDONLY;
  }
  else {
    mode = FIX2INT(opt_mode);
    if (mode < 0 || mode > 2) {
      rb_raise(rb_eTypeError, "mode must be 0, 1, or 2");
    }
  }

  // remote address
  opt_remote = rb_hash_aref(options, sym_remote);
  if (NIL_P(opt_remote)) {
    rb_raise(rb_eRuntimeError, "remote is required");
  }
  if (TYPE(opt_remote) != T_STRING) {
    rb_raise(rb_eTypeError, "remote must be a string");
  }
  str  = RSTRING_PTR(opt_remote);
  len  = RSTRING_LEN(opt_remote);
  sptr = index(str, ':');
  if (sptr == NULL) {
    rb_raise(rb_eRuntimeError, "remote must have a valid port");
  }
  len2 = (int)(sptr - str);
  remote_addr = rb_str_new(str, len2);
  remote_port = atoi(sptr+1);
  if (remote_port <= 0 || remote_port > 65535) {
    rb_raise(rb_eRuntimeError, "remote must have a valid port");
  }
  if (!RTEST(rb_reg_match(ip_regexp, remote_addr))) {
    rb_raise(rb_eRuntimeError, "remote must have a valid IP");
  }
  rb_iv_set(self, "@remote_addr", remote_addr);
  rb_iv_set(self, "@remote_port", INT2FIX(remote_port));

  // local address; isn't required for SENDONLY
  opt_local = rb_hash_aref(options, sym_local);
  if (!NIL_P(opt_local)) {
    if (TYPE(opt_local) != T_STRING) {
      rb_raise(rb_eTypeError, "local must be a string");
    }
    str  = RSTRING_PTR(opt_local);
    len  = RSTRING_LEN(opt_local);
    sptr = index(str, ':');
    if (sptr == NULL) {
      // port is not required for local connection
      local_port = -1;
      local_addr = rb_str_new(str, len);
      rb_iv_set(self, "@local_port", Qnil);
    }
    else {
      len2 = (int)(sptr - str);
      local_addr = rb_str_new(str, len2);
      local_port = atoi(sptr+1);
      if (local_port <= 0 || local_port > 65535) {
        rb_raise(rb_eRuntimeError, "local must have a valid port");
      }
      rb_iv_set(self, "@local_port", INT2FIX(local_port));
    }
    if (!RTEST(rb_reg_match(ip_regexp, local_addr))) {
      rb_raise(rb_eRuntimeError, "local must have a valid IP");
    }
    rb_iv_set(self, "@local_addr", local_addr);
  }
  else {
    if (NIL_P(opt_local) && mode != RTP_SESSION_SENDONLY) {
      rb_raise(rb_eRuntimeError, "local is required");
    }
    local_addr = Qnil;
  }

  session = rtp_session_new(mode);
  if (RTEST(local_addr)) {
    rtp_session_set_local_addr(session, RSTRING_PTR(local_addr), local_port);
  }
  rtp_session_set_remote_addr(session, RSTRING_PTR(remote_addr), remote_port);
  DATA_PTR(self) = session;
  return self;
}

static void
rtp_shutdown(self)
  VALUE self;
{
  ortp_exit();
}

void
Init_rtp()
{
  ortp_init();

  sym_mode = ID2SYM(rb_intern("mode"));
  sym_remote = ID2SYM(rb_intern("remote"));
  sym_local = ID2SYM(rb_intern("local"));

  rb_mRTP = rb_define_module("RTP");
  rb_cRSession = rb_define_class_under(rb_mRTP, "Session", rb_cObject);

  rb_define_const(rb_cRSession, "RTP_SESSION_RECVONLY", INT2FIX(RTP_SESSION_RECVONLY));
  rb_define_const(rb_cRSession, "RTP_SESSION_SENDONLY", INT2FIX(RTP_SESSION_SENDONLY));
  rb_define_const(rb_cRSession, "RTP_SESSION_SENDRECV", INT2FIX(RTP_SESSION_SENDRECV));
  ip_regexp = rb_reg_new("^(?:\\d{1,3}\\.){3}\\d{1,3}$", 25, 0);
  rb_define_const(rb_cRSession, "IP_REGEXP", ip_regexp);

  rb_define_alloc_func(rb_cRSession, session_alloc);
  rb_define_method(rb_cRSession, "initialize", session_init, -1);
  rb_define_attr(rb_cRSession, "remote_addr", 1, 0);
  rb_define_attr(rb_cRSession, "remote_port", 1, 0);
  rb_define_attr(rb_cRSession, "local_addr", 1, 0);
  rb_define_attr(rb_cRSession, "local_port", 1, 0);

  rb_set_end_proc(rtp_shutdown, 0);
}
