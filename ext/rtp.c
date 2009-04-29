#include <ortp/ortp.h>
#include <ortp/rtp.h>
#include <ortp/rtpsession.h>
#include "ruby.h"

VALUE rb_mRTP;
VALUE rb_cRSession;
VALUE sym_mode, sym_remote, sym_local, sym_block, sym_connected;
VALUE ip_regexp;

int scheduler_started = 0;

static void
start_scheduler()
{
  if (!scheduler_started) {
    ortp_scheduler_init();
    scheduler_started = 1;
  }
}

static void
parse_address(which, str, addr, port)
  char *which;
  VALUE str, *addr, *port;
{
  char *str_ptr, *idx_ptr;
  int   str_len,  idx_len, num;

  if (!NIL_P(str)) {
    if (TYPE(str) != T_STRING) {
      rb_raise(rb_eTypeError, "%s must be a string", which);
    }
    str_ptr = RSTRING_PTR(str);
    str_len = RSTRING_LEN(str);
    idx_ptr = index(str_ptr, ':');
    if (idx_ptr == NULL) {
      *port = Qnil;
      *addr = rb_str_new(str_ptr, str_len);
    }
    else {
      num = atoi(idx_ptr+1);
      if (num == -1) {
        *port = Qnil;
      }
      else if (num <= 0 || num > 65535) {
        rb_raise(rb_eRuntimeError, "%s port is invalid", which);
      }
      else {
        *port = INT2FIX(num);
      }

      idx_len = (int)(idx_ptr - str_ptr);
      *addr = rb_str_new(str_ptr, idx_len);
      if (!RTEST(rb_reg_match(ip_regexp, *addr))) {
        rb_raise(rb_eRuntimeError, "%s IP is invalid", which);
      }
    }
  }
  else {
    *addr = *port = Qnil;
  }
}

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
  VALUE options, opt_mode, opt_remote, opt_local, opt_block,
        opt_connected;
  VALUE remote_addr, remote_port, local_addr, local_port;
  int mode;
  RtpSession *session;

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
  parse_address("remote", opt_remote, &remote_addr, &remote_port);
  if (mode != RTP_SESSION_RECVONLY) {
    if (NIL_P(remote_addr))
      rb_raise(rb_eRuntimeError, "remote address and port are required");
    if (NIL_P(remote_port)) {
      rb_raise(rb_eRuntimeError, "remote port is required");
    }
  }
  rb_iv_set(self, "@remote_addr", remote_addr);
  rb_iv_set(self, "@remote_port", remote_port);

  // local address
  opt_local = rb_hash_aref(options, sym_local);
  parse_address("local", opt_local, &local_addr, &local_port);
  if (mode != RTP_SESSION_SENDONLY) {
    if (NIL_P(local_addr))
      rb_raise(rb_eRuntimeError, "local address and port are required");
    if (NIL_P(local_port)) {
      rb_raise(rb_eRuntimeError, "local port is required");
    }
  }
  rb_iv_set(self, "@local_addr", local_addr);
  rb_iv_set(self, "@local_port", local_port);

  // blocking
  opt_block = rb_hash_aref(options, sym_block);

  // connected
  opt_connected = rb_hash_aref(options, sym_connected);

  // session time!
  session = rtp_session_new(mode);
  if (RTEST(local_addr)) {
    rtp_session_set_local_addr(session, RSTRING_PTR(local_addr), FIX2INT(local_port));
  }
  if (RTEST(remote_addr)) {
    rtp_session_set_remote_addr(session, RSTRING_PTR(remote_addr), FIX2INT(remote_port));
  }
  if (RTEST(opt_block)) {
    start_scheduler();
    rtp_session_set_blocking_mode(session, 1);
  }
  else {
    rtp_session_set_blocking_mode(session, 0);
  }
  rtp_session_set_connected_mode(session, RTEST(opt_connected));
  rtp_session_set_payload_type(session, 0);
  DATA_PTR(self) = session;
  return self;
}

static VALUE
session_closed(self)
  VALUE self;
{
  VALUE closed = rb_iv_get(self, "@closed");
  if (RTEST(closed)) {
    return Qtrue;
  }
  return Qfalse;
}

static VALUE
session_close(self)
  VALUE self;
{
  RtpSession *session;
  if (!RTEST(session_closed(self))) {
    Data_Get_Struct(self, RtpSession, session);
    //rtp_session_bye(session, "Closing...");
    rtp_session_release_sockets(session);
    rb_iv_set(self, "@closed", Qtrue);
  }
  return Qnil;
}

static VALUE
session_send_file(self, filename)
  VALUE self, filename;
{
  unsigned char buffer[160];
  int num;
  uint32_t ts = 0;

  RtpSession *session;
  Data_Get_Struct(self, RtpSession, session);

  FILE *infile = fopen(RSTRING_PTR(filename), "r");
  while ((num = fread(buffer, 1, 160, infile)) > 0) {
    rtp_session_send_with_ts(session, buffer, num, ts);
    ts += 160;
  }

  fclose(infile);
  return Qnil;
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
  sym_block = ID2SYM(rb_intern("block"));

  rb_mRTP = rb_define_module("RTP");
  rb_cRSession = rb_define_class_under(rb_mRTP, "Session", rb_cObject);

  rb_define_const(rb_cRSession, "RTP_SESSION_RECVONLY", INT2FIX(RTP_SESSION_RECVONLY));
  rb_define_const(rb_cRSession, "RTP_SESSION_SENDONLY", INT2FIX(RTP_SESSION_SENDONLY));
  rb_define_const(rb_cRSession, "RTP_SESSION_SENDRECV", INT2FIX(RTP_SESSION_SENDRECV));
  ip_regexp = rb_reg_new("^(?:\\d{1,3}\\.){3}\\d{1,3}$", 25, 0);
  rb_define_const(rb_cRSession, "IP_REGEXP", ip_regexp);

  rb_define_alloc_func(rb_cRSession, session_alloc);
  rb_define_method(rb_cRSession, "initialize", session_init, -1);
  rb_define_method(rb_cRSession, "close", session_close, 0);
  rb_define_method(rb_cRSession, "closed?", session_closed, 0);
  rb_define_method(rb_cRSession, "send_file", session_send_file, 1);
  rb_define_attr(rb_cRSession, "remote_addr", 1, 0);
  rb_define_attr(rb_cRSession, "remote_port", 1, 0);
  rb_define_attr(rb_cRSession, "local_addr", 1, 0);
  rb_define_attr(rb_cRSession, "local_port", 1, 0);

  rb_set_end_proc(rtp_shutdown, 0);
}
