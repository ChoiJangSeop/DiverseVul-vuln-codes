mrb_proc_init_copy(mrb_state *mrb, mrb_value self)
{
  mrb_value proc = mrb_get_arg1(mrb);

  if (!mrb_proc_p(proc)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "not a proc");
  }
  mrb_proc_copy(mrb_proc_ptr(self), mrb_proc_ptr(proc));
  return self;
}