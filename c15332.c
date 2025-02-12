mrb_proc_s_new(mrb_state *mrb, mrb_value proc_class)
{
  mrb_value blk;
  mrb_value proc;
  struct RProc *p;

  /* Calling Proc.new without a block is not implemented yet */
  mrb_get_args(mrb, "&!", &blk);
  p = MRB_OBJ_ALLOC(mrb, MRB_TT_PROC, mrb_class_ptr(proc_class));
  mrb_proc_copy(p, mrb_proc_ptr(blk));
  proc = mrb_obj_value(p);
  mrb_funcall_with_block(mrb, proc, MRB_SYM(initialize), 0, NULL, proc);
  if (!MRB_PROC_STRICT_P(p) &&
      mrb->c->ci > mrb->c->cibase && MRB_PROC_ENV(p) == mrb->c->ci[-1].u.env) {
    p->flags |= MRB_PROC_ORPHAN;
  }
  return proc;
}