#include "mxnet_internal.h"

/* Compute the gradients of heads w.r.t previously marked variables.
 */
static VALUE
autograd_s_backward(int argc, VALUE *argv, VALUE mod)
{
  VALUE heads, opts, head_grads = Qnil;
  int retain_graphs = 0, train_mode = 1;

  VALUE head_handles_str, head_grad_handles_str;
  NDArrayHandle *head_handles, *head_grad_handles = NULL;
  mx_uint i, heads_len;

  rb_scan_args(argc, argv, "1:", &heads, &opts);
  if (!NIL_P(opts)) {
    static ID keywords[3];
    VALUE vals[3];

    if (keywords[0] == 0) {
      keywords[0] = rb_intern("head_grads");
      keywords[1] = rb_intern("retain_graph");
      keywords[2] = rb_intern("train_mode");
    }

    rb_get_kwargs(opts, keywords, 0, 3, vals);

    if (vals[0] != Qundef) {
      head_grads = vals[0];
    }
    if (vals[1] != Qundef) {
      retain_graphs = RTEST(vals[1]);
    }
    if (vals[2] != Qundef) {
      train_mode = RTEST(vals[2]);
    }
  }

  if (mxnet_is_ndarray(heads)) {
    VALUE ary = rb_ary_new_capa(1);
    rb_ary_push(ary, heads);
    heads = ary;
  }

  heads = rb_convert_type(heads, T_ARRAY, "Array", "to_ary");
#if SIZEOF_LONG > SIZEOF_INT
  if (RARRAY_LEN(heads) > UINT_MAX) {
    rb_raise(rb_eArgError, "too many heads");
  }
#endif

  heads_len = (mx_uint)RARRAY_LEN(heads);
  head_handles_str = rb_str_tmp_new(sizeof(NDArrayHandle) * heads_len);
  head_handles = (NDArrayHandle *)RSTRING_PTR(head_handles_str);
  for (i = 0; i < heads_len; ++i) {
    head_handles[i] = mxnet_ndarray_get_handle(RARRAY_AREF(heads, i));
  }

  if (!NIL_P(head_grads)) {
    if (mxnet_is_ndarray(head_grads)) {
      VALUE ary = rb_ary_new_capa(1);
      rb_ary_push(ary, head_grads);
      head_grads = ary;
    }

    head_grads = rb_convert_type(head_grads, T_ARRAY, "Array", "to_ary");
    if (RARRAY_LEN(heads) != RARRAY_LEN(head_grads)) {
      rb_raise(rb_eArgError, "haeds and head_grads must be arrays of the same length");
    }

    head_grad_handles_str = rb_str_tmp_new(sizeof(NDArrayHandle) * heads_len);
    head_grad_handles = (NDArrayHandle *)RSTRING_PTR(head_grad_handles_str);
    for (i = 0; i < heads_len; ++i) {
      head_grad_handles[i] = mxnet_ndarray_get_handle(RARRAY_AREF(head_grads, i));
    }
  }

  CHECK_CALL(
    MXNET_API(MXAutogradBackwardEx)(
      heads_len,
      head_handles,
      head_grad_handles,
      0,
      NULL,
      retain_graphs,
      0,
      train_mode,
      NULL,
      NULL));

  return Qnil;
}

/* Set status to recording/not recording.
 * When recording, graph will be constructed for gradient computation.
 *
 * @param is_recording [true, false]
 * @return [true, false] The previous state before this set.
 */
static VALUE
autograd_s_set_recording(VALUE mod, VALUE is_recording)
{
  int prev;
  CHECK_CALL(MXNET_API(MXAutogradSetIsRecording)(RTEST(is_recording), &prev));
  return prev ? Qtrue : Qfalse;
}

/* Set status to training/predicting.  This affects ctx.is_train in operator
 * running context.  For example, Dropout will drop inputs randomly when
 * train_mode is true while simply passing through if train_mode is false.
 *
 * @param train_mode [true, false]
 * @return [true, false] The previous state before this set.
 */
static VALUE
autograd_s_set_training(VALUE mod, VALUE train_mode)
{
  int prev;
  CHECK_CALL(MXNET_API(MXAutogradSetIsTraining)(RTEST(train_mode), &prev));
  return prev ? Qtrue : Qfalse;
}

static VALUE
autograd_s_recording_p(VALUE mod)
{
  bool curr;
  CHECK_CALL(MXNET_API(MXAutogradIsRecording)(&curr));
  return curr ? Qtrue : Qfalse;
}

static VALUE
autograd_s_training_p(VALUE mod)
{
  bool curr;
  CHECK_CALL(MXNET_API(MXAutogradIsTraining)(&curr));
  return curr ? Qtrue : Qfalse;
}

void
mxnet_init_autograd(void)
{
  VALUE mAutograd;

  mAutograd = rb_const_get_at(mxnet_mMXNet, rb_intern("Autograd"));
  rb_define_singleton_method(mAutograd, "backward", autograd_s_backward, -1);
  rb_define_singleton_method(mAutograd, "set_recording", autograd_s_set_recording, 1);
  rb_define_singleton_method(mAutograd, "set_training", autograd_s_set_training, 1);
  rb_define_singleton_method(mAutograd, "recording?", autograd_s_recording_p, 0);
  rb_define_singleton_method(mAutograd, "training?", autograd_s_training_p, 0);
}
