#include <ruby.h>
#include "sd_journal.h"

void Init_journald_native();

/* initializers */
static void jdl_init_modules();
static void jdl_init_constants();
static void jdl_init_methods();

/* methods */
static VALUE jdl_native_print(VALUE self, VALUE priority, VALUE message);
static VALUE jdl_native_send(int argc, VALUE* argv, VALUE self);
static VALUE jdl_native_perror(VALUE self, VALUE message);
static VALUE jdl_native_is_dummy();

/* globals */
static VALUE mJournald;
static VALUE mNative;

void Init_journald_native()
{
    jdl_init_modules();
    jdl_init_constants();
    jdl_init_methods();
}

static void jdl_init_modules()
{
    mJournald = rb_define_module("Journald");
    mNative   = rb_define_module_under(mJournald, "Native");
}

static void jdl_init_constants()
{
    rb_define_const(mJournald, "LOG_EMERG",   INT2NUM(LOG_EMERG));    /* system is unusable */
    rb_define_const(mJournald, "LOG_ALERT",   INT2NUM(LOG_ALERT));    /* action must be taken immediately */
    rb_define_const(mJournald, "LOG_CRIT",    INT2NUM(LOG_CRIT));     /* critical conditions */
    rb_define_const(mJournald, "LOG_ERR",     INT2NUM(LOG_ERR));      /* error conditions */
    rb_define_const(mJournald, "LOG_WARNING", INT2NUM(LOG_WARNING));  /* warning conditions */
    rb_define_const(mJournald, "LOG_NOTICE",  INT2NUM(LOG_NOTICE));   /* normal but significant condition */
    rb_define_const(mJournald, "LOG_INFO",    INT2NUM(LOG_INFO));     /* informational */
    rb_define_const(mJournald, "LOG_DEBUG",   INT2NUM(LOG_DEBUG));    /* debug-level messages */

    // dummy detection const
    rb_define_const(mNative,   "IS_DUMMY",    jdl_native_is_dummy());
}

static void jdl_init_methods()
{
    rb_define_singleton_method(mNative, "print",  jdl_native_print,  2);
    rb_define_singleton_method(mNative, "send",   jdl_native_send,  -1); /* -1 to pass as C array */
    rb_define_singleton_method(mNative, "perror", jdl_native_perror, 1);

    // dummy detection method
    rb_define_singleton_method(mNative, "dummy?", jdl_native_is_dummy, 0);
}

static VALUE jdl_native_print(VALUE v_self, VALUE v_priority, VALUE v_message)
{
    int priority, result;
    char *message;

    priority = NUM2INT(v_priority);
    message  = StringValueCStr(v_message);

    result = sd_journal_print(priority, "%s", message);

    return INT2NUM(result);
}

static VALUE jdl_native_send(int argc, VALUE* argv, VALUE self)
{
    struct iovec *msgs;
    int i;
    int result;

    /* first check everything is a string / convertable to string */
    for (i = 0; i < argc; i++) {
        StringValue(argv[i]); /* you may get a ruby exception here */
    }

    /* allocate memory after all checks to avoid possible memory leak */
    msgs = calloc(argc, sizeof(struct iovec));

    for (i = 0; i < argc; i++) {
        VALUE v = argv[i];
        msgs[i].iov_base = RSTRING_PTR(v);
        msgs[i].iov_len  = RSTRING_LEN(v);
    }

    result = sd_journal_sendv(msgs, argc);

    free(msgs);

    return INT2NUM(result);
}

static VALUE jdl_native_perror(VALUE v_self, VALUE v_message)
{
    int result;
    char *message;

    message = StringValueCStr(v_message);

    result = sd_journal_perror(message);

    return INT2NUM(result);
}

static VALUE jdl_native_is_dummy()
{
    return JOURNALD_NATIVE_SD_JOURNAL_DUMMY ? Qtrue : Qfalse;
}