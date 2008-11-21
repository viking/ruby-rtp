#include <ortp/ortp.h>
#include <ortp/rtp.h>
#include <ortp/rtpsession.h>
#include "ruby.h"

VALUE rb_mRTP;
VALUE rb_cRSession;
VALUE sym_mode;

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
  VALUE options, opt_mode;
  RtpSession *session;
  int mode;

  rb_scan_args(argc, argv, "01", &options);
  if (NIL_P(options)) {
    options = rb_hash_new();
  }

  // session mode
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

  session = rtp_session_new(mode);
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

  rb_mRTP = rb_define_module("RTP");
  rb_cRSession = rb_define_class_under(rb_mRTP, "Session", rb_cObject);

  rb_define_const(rb_cRSession, "RTP_SESSION_RECVONLY", INT2FIX(RTP_SESSION_RECVONLY));
  rb_define_const(rb_cRSession, "RTP_SESSION_SENDONLY", INT2FIX(RTP_SESSION_SENDONLY));
  rb_define_const(rb_cRSession, "RTP_SESSION_SENDRECV", INT2FIX(RTP_SESSION_SENDRECV));

  rb_define_alloc_func(rb_cRSession, session_alloc);
  rb_define_method(rb_cRSession, "initialize", session_init, -1);

  rb_set_end_proc(rtp_shutdown, 0);
}
