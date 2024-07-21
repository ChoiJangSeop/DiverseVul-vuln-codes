mrb_f_send(mrb_state *mrb, mrb_value self)
{
  mrb_sym name;
  mrb_value block, *regs;
  mrb_method_t m;
  struct RClass *c;
  mrb_callinfo *ci = mrb->c->ci;
  int n = ci->n;

  if (ci->cci > CINFO_NONE) {
  funcall:;
    const mrb_value *argv;
    mrb_int argc;
    mrb_get_args(mrb, "n*&", &name, &argv, &argc, &block);
    return mrb_funcall_with_block(mrb, self, name, argc, argv, block);
  }

  regs = mrb->c->ci->stack+1;

  if (n == 0) {
    mrb_argnum_error(mrb, 0, 1, -1);
  }
  else if (n == 15) {
    name = mrb_obj_to_sym(mrb, RARRAY_PTR(regs[0])[0]);
  }
  else {
    name = mrb_obj_to_sym(mrb, regs[0]);
  }

  c = mrb_class(mrb, self);
  m = mrb_method_search_vm(mrb, &c, name);
  if (MRB_METHOD_UNDEF_P(m)) {            /* call method_mising */
    goto funcall;
  }

  ci->mid = name;
  ci->u.target_class = c;
  /* remove first symbol from arguments */
  if (n == 15) {     /* variable length arguments */
    regs[0] = mrb_ary_subseq(mrb, regs[0], 1, RARRAY_LEN(regs[0]) - 1);
  }
  else { /* n > 0 */
    for (int i=0; i<n; i++) {
      regs[i] = regs[i+1];
    }
    regs[n] = regs[n+1];        /* copy kdict or block */
    if (ci->nk > 0) {
      regs[n+1] = regs[n+2];    /* copy block */
    }
    ci->n--;
  }

  if (MRB_METHOD_CFUNC_P(m)) {
    if (MRB_METHOD_NOARG_P(m)) {
      check_method_noarg(mrb, ci);
    }

    if (MRB_METHOD_PROC_P(m)) {
      mrb_vm_ci_proc_set(ci, MRB_METHOD_PROC(m));
    }
    return MRB_METHOD_CFUNC(m)(mrb, self);
  }
  return exec_irep(mrb, self, MRB_METHOD_PROC(m));
}