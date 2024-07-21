mrb_vm_exec(mrb_state *mrb, const struct RProc *proc, const mrb_code *pc)
{
  /* mrb_assert(MRB_PROC_CFUNC_P(proc)) */
  const mrb_irep *irep = proc->body.irep;
  const mrb_pool_value *pool = irep->pool;
  const mrb_sym *syms = irep->syms;
  mrb_code insn;
  int ai = mrb_gc_arena_save(mrb);
  struct mrb_jmpbuf *prev_jmp = mrb->jmp;
  struct mrb_jmpbuf c_jmp;
  uint32_t a;
  uint16_t b;
  uint16_t c;
  mrb_sym mid;
  const struct mrb_irep_catch_handler *ch;

#ifdef DIRECT_THREADED
  static const void * const optable[] = {
#define OPCODE(x,_) &&L_OP_ ## x,
#include "mruby/ops.h"
#undef OPCODE
  };
#endif

  mrb_bool exc_catched = FALSE;
RETRY_TRY_BLOCK:

  MRB_TRY(&c_jmp) {

  if (exc_catched) {
    exc_catched = FALSE;
    mrb_gc_arena_restore(mrb, ai);
    if (mrb->exc && mrb->exc->tt == MRB_TT_BREAK)
      goto L_BREAK;
    goto L_RAISE;
  }
  mrb->jmp = &c_jmp;
  mrb_vm_ci_proc_set(mrb->c->ci, proc);

#define regs (mrb->c->ci->stack)
  INIT_DISPATCH {
    CASE(OP_NOP, Z) {
      /* do nothing */
      NEXT;
    }

    CASE(OP_MOVE, BB) {
      regs[a] = regs[b];
      NEXT;
    }

    CASE(OP_LOADL, BB) {
      switch (pool[b].tt) {   /* number */
      case IREP_TT_INT32:
        regs[a] = mrb_int_value(mrb, (mrb_int)pool[b].u.i32);
        break;
      case IREP_TT_INT64:
#if defined(MRB_INT64)
        regs[a] = mrb_int_value(mrb, (mrb_int)pool[b].u.i64);
        break;
#else
#if defined(MRB_64BIT)
        if (INT32_MIN <= pool[b].u.i64 && pool[b].u.i64 <= INT32_MAX) {
          regs[a] = mrb_int_value(mrb, (mrb_int)pool[b].u.i64);
          break;
        }
#endif
        goto L_INT_OVERFLOW;
#endif
      case IREP_TT_BIGINT:
        goto L_INT_OVERFLOW;
#ifndef MRB_NO_FLOAT
      case IREP_TT_FLOAT:
        regs[a] = mrb_float_value(mrb, pool[b].u.f);
        break;
#endif
      default:
        /* should not happen (tt:string) */
        regs[a] = mrb_nil_value();
        break;
      }
      NEXT;
    }

    CASE(OP_LOADI, BB) {
      SET_FIXNUM_VALUE(regs[a], b);
      NEXT;
    }

    CASE(OP_LOADINEG, BB) {
      SET_FIXNUM_VALUE(regs[a], -b);
      NEXT;
    }

    CASE(OP_LOADI__1,B) goto L_LOADI;
    CASE(OP_LOADI_0,B) goto L_LOADI;
    CASE(OP_LOADI_1,B) goto L_LOADI;
    CASE(OP_LOADI_2,B) goto L_LOADI;
    CASE(OP_LOADI_3,B) goto L_LOADI;
    CASE(OP_LOADI_4,B) goto L_LOADI;
    CASE(OP_LOADI_5,B) goto L_LOADI;
    CASE(OP_LOADI_6,B) goto L_LOADI;
    CASE(OP_LOADI_7, B) {
    L_LOADI:
      SET_FIXNUM_VALUE(regs[a], (mrb_int)insn - (mrb_int)OP_LOADI_0);
      NEXT;
    }

    CASE(OP_LOADI16, BS) {
      SET_FIXNUM_VALUE(regs[a], (mrb_int)(int16_t)b);
      NEXT;
    }

    CASE(OP_LOADI32, BSS) {
      SET_INT_VALUE(mrb, regs[a], (int32_t)(((uint32_t)b<<16)+c));
      NEXT;
    }

    CASE(OP_LOADSYM, BB) {
      SET_SYM_VALUE(regs[a], syms[b]);
      NEXT;
    }

    CASE(OP_LOADNIL, B) {
      SET_NIL_VALUE(regs[a]);
      NEXT;
    }

    CASE(OP_LOADSELF, B) {
      regs[a] = regs[0];
      NEXT;
    }

    CASE(OP_LOADT, B) {
      SET_TRUE_VALUE(regs[a]);
      NEXT;
    }

    CASE(OP_LOADF, B) {
      SET_FALSE_VALUE(regs[a]);
      NEXT;
    }

    CASE(OP_GETGV, BB) {
      mrb_value val = mrb_gv_get(mrb, syms[b]);
      regs[a] = val;
      NEXT;
    }

    CASE(OP_SETGV, BB) {
      mrb_gv_set(mrb, syms[b], regs[a]);
      NEXT;
    }

    CASE(OP_GETSV, BB) {
      mrb_value val = mrb_vm_special_get(mrb, syms[b]);
      regs[a] = val;
      NEXT;
    }

    CASE(OP_SETSV, BB) {
      mrb_vm_special_set(mrb, syms[b], regs[a]);
      NEXT;
    }

    CASE(OP_GETIV, BB) {
      regs[a] = mrb_iv_get(mrb, regs[0], syms[b]);
      NEXT;
    }

    CASE(OP_SETIV, BB) {
      mrb_iv_set(mrb, regs[0], syms[b], regs[a]);
      NEXT;
    }

    CASE(OP_GETCV, BB) {
      mrb_value val;
      val = mrb_vm_cv_get(mrb, syms[b]);
      regs[a] = val;
      NEXT;
    }

    CASE(OP_SETCV, BB) {
      mrb_vm_cv_set(mrb, syms[b], regs[a]);
      NEXT;
    }

    CASE(OP_GETIDX, B) {
      mrb_value va = regs[a], vb = regs[a+1];
      switch (mrb_type(va)) {
      case MRB_TT_ARRAY:
        if (!mrb_integer_p(vb)) goto getidx_fallback;
        regs[a] = mrb_ary_entry(va, mrb_integer(vb));
        break;
      case MRB_TT_HASH:
        va = mrb_hash_get(mrb, va, vb);
        regs[a] = va;
        break;
      case MRB_TT_STRING:
        switch (mrb_type(vb)) {
        case MRB_TT_INTEGER:
        case MRB_TT_STRING:
        case MRB_TT_RANGE:
          va = mrb_str_aref(mrb, va, vb, mrb_undef_value());
          regs[a] = va;
          break;
        default:
          goto getidx_fallback;
        }
        break;
      default:
      getidx_fallback:
        mid = MRB_OPSYM(aref);
        goto L_SEND_SYM;
      }
      NEXT;
    }

    CASE(OP_SETIDX, B) {
      c = 2;
      mid = MRB_OPSYM(aset);
      SET_NIL_VALUE(regs[a+3]);
      goto L_SENDB_SYM;
    }

    CASE(OP_GETCONST, BB) {
      mrb_value v = mrb_vm_const_get(mrb, syms[b]);
      regs[a] = v;
      NEXT;
    }

    CASE(OP_SETCONST, BB) {
      mrb_vm_const_set(mrb, syms[b], regs[a]);
      NEXT;
    }

    CASE(OP_GETMCNST, BB) {
      mrb_value v = mrb_const_get(mrb, regs[a], syms[b]);
      regs[a] = v;
      NEXT;
    }

    CASE(OP_SETMCNST, BB) {
      mrb_const_set(mrb, regs[a+1], syms[b], regs[a]);
      NEXT;
    }

    CASE(OP_GETUPVAR, BBB) {
      mrb_value *regs_a = regs + a;
      struct REnv *e = uvenv(mrb, c);

      if (e && b < MRB_ENV_LEN(e)) {
        *regs_a = e->stack[b];
      }
      else {
        *regs_a = mrb_nil_value();
      }
      NEXT;
    }

    CASE(OP_SETUPVAR, BBB) {
      struct REnv *e = uvenv(mrb, c);

      if (e) {
        mrb_value *regs_a = regs + a;

        if (b < MRB_ENV_LEN(e)) {
          e->stack[b] = *regs_a;
          mrb_write_barrier(mrb, (struct RBasic*)e);
        }
      }
      NEXT;
    }

    CASE(OP_JMP, S) {
      pc += (int16_t)a;
      JUMP;
    }
    CASE(OP_JMPIF, BS) {
      if (mrb_test(regs[a])) {
        pc += (int16_t)b;
        JUMP;
      }
      NEXT;
    }
    CASE(OP_JMPNOT, BS) {
      if (!mrb_test(regs[a])) {
        pc += (int16_t)b;
        JUMP;
      }
      NEXT;
    }
    CASE(OP_JMPNIL, BS) {
      if (mrb_nil_p(regs[a])) {
        pc += (int16_t)b;
        JUMP;
      }
      NEXT;
    }

    CASE(OP_JMPUW, S) {
      a = (uint32_t)((pc - irep->iseq) + (int16_t)a);
      CHECKPOINT_RESTORE(RBREAK_TAG_JUMP) {
        struct RBreak *brk = (struct RBreak*)mrb->exc;
        mrb_value target = mrb_break_value_get(brk);
        mrb_assert(mrb_integer_p(target));
        a = (uint32_t)mrb_integer(target);
        mrb_assert(a >= 0 && a < irep->ilen);
      }
      CHECKPOINT_MAIN(RBREAK_TAG_JUMP) {
        ch = catch_handler_find(mrb, mrb->c->ci, pc, MRB_CATCH_FILTER_ENSURE);
        if (ch) {
          /* avoiding a jump from a catch handler into the same handler */
          if (a < mrb_irep_catch_handler_unpack(ch->begin) || a >= mrb_irep_catch_handler_unpack(ch->end)) {
            THROW_TAGGED_BREAK(mrb, RBREAK_TAG_JUMP, proc, mrb_fixnum_value(a));
          }
        }
      }
      CHECKPOINT_END(RBREAK_TAG_JUMP);

      mrb->exc = NULL; /* clear break object */
      pc = irep->iseq + a;
      JUMP;
    }

    CASE(OP_EXCEPT, B) {
      mrb_value exc;

      if (mrb->exc == NULL) {
        exc = mrb_nil_value();
      }
      else {
        switch (mrb->exc->tt) {
        case MRB_TT_BREAK:
        case MRB_TT_EXCEPTION:
          exc = mrb_obj_value(mrb->exc);
          break;
        default:
          mrb_assert(!"bad mrb_type");
          exc = mrb_nil_value();
          break;
        }
        mrb->exc = NULL;
      }
      regs[a] = exc;
      NEXT;
    }
    CASE(OP_RESCUE, BB) {
      mrb_value exc = regs[a];  /* exc on stack */
      mrb_value e = regs[b];
      struct RClass *ec;

      switch (mrb_type(e)) {
      case MRB_TT_CLASS:
      case MRB_TT_MODULE:
        break;
      default:
        {
          mrb_value exc;

          exc = mrb_exc_new_lit(mrb, E_TYPE_ERROR,
                                    "class or module required for rescue clause");
          mrb_exc_set(mrb, exc);
          goto L_RAISE;
        }
      }
      ec = mrb_class_ptr(e);
      regs[b] = mrb_bool_value(mrb_obj_is_kind_of(mrb, exc, ec));
      NEXT;
    }

    CASE(OP_RAISEIF, B) {
      mrb_value exc = regs[a];
      if (mrb_break_p(exc)) {
        mrb->exc = mrb_obj_ptr(exc);
        goto L_BREAK;
      }
      mrb_exc_set(mrb, exc);
      if (mrb->exc) {
        goto L_RAISE;
      }
      NEXT;
    }

    CASE(OP_SSEND, BBB) {
      regs[a] = regs[0];
      insn = OP_SEND;
    }
    goto L_SENDB;

    CASE(OP_SSENDB, BBB) {
      regs[a] = regs[0];
    }
    goto L_SENDB;

    CASE(OP_SEND, BBB)
    goto L_SENDB;

    L_SEND_SYM:
    c = 1;
    /* push nil after arguments */
    SET_NIL_VALUE(regs[a+2]);
    goto L_SENDB_SYM;

    CASE(OP_SENDB, BBB)
    L_SENDB:
    mid = syms[b];
    L_SENDB_SYM:
    {
      mrb_callinfo *ci = mrb->c->ci;
      mrb_method_t m;
      struct RClass *cls;
      mrb_value recv, blk;

      ARGUMENT_NORMALIZE(a, &c, insn);

      recv = regs[a];
      cls = mrb_class(mrb, recv);
      m = mrb_method_search_vm(mrb, &cls, mid);
      if (MRB_METHOD_UNDEF_P(m)) {
        m = prepare_missing(mrb, recv, mid, &cls, a, &c, blk, 0);
        mid = MRB_SYM(method_missing);
      }

      /* push callinfo */
      ci = cipush(mrb, a, 0, cls, NULL, mid, c);

      if (MRB_METHOD_CFUNC_P(m)) {
        if (MRB_METHOD_PROC_P(m)) {
          struct RProc *p = MRB_METHOD_PROC(m);

          mrb_vm_ci_proc_set(ci, p);
          recv = p->body.func(mrb, recv);
        }
        else {
          if (MRB_METHOD_NOARG_P(m)) {
            check_method_noarg(mrb, ci);
          }
          recv = MRB_METHOD_FUNC(m)(mrb, recv);
        }
        mrb_gc_arena_shrink(mrb, ai);
        if (mrb->exc) goto L_RAISE;
        ci = mrb->c->ci;
        if (mrb_proc_p(blk)) {
          struct RProc *p = mrb_proc_ptr(blk);
          if (p && !MRB_PROC_STRICT_P(p) && MRB_PROC_ENV(p) == mrb_vm_ci_env(&ci[-1])) {
            p->flags |= MRB_PROC_ORPHAN;
          }
        }
        if (!ci->u.target_class) { /* return from context modifying method (resume/yield) */
          if (ci->cci == CINFO_RESUMED) {
            mrb->jmp = prev_jmp;
            return recv;
          }
          else {
            mrb_assert(!MRB_PROC_CFUNC_P(ci[-1].proc));
            proc = ci[-1].proc;
            irep = proc->body.irep;
            pool = irep->pool;
            syms = irep->syms;
          }
        }
        ci->stack[0] = recv;
        /* pop stackpos */
        ci = cipop(mrb);
        pc = ci->pc;
      }
      else {
        /* setup environment for calling method */
        mrb_vm_ci_proc_set(ci, (proc = MRB_METHOD_PROC(m)));
        irep = proc->body.irep;
        pool = irep->pool;
        syms = irep->syms;
        mrb_stack_extend(mrb, (irep->nregs < 4) ? 4 : irep->nregs);
        pc = irep->iseq;
      }
    }
    JUMP;

    CASE(OP_CALL, Z) {
      mrb_callinfo *ci = mrb->c->ci;
      mrb_value recv = ci->stack[0];
      struct RProc *m = mrb_proc_ptr(recv);

      /* replace callinfo */
      ci->u.target_class = MRB_PROC_TARGET_CLASS(m);
      mrb_vm_ci_proc_set(ci, m);
      if (MRB_PROC_ENV_P(m)) {
        ci->mid = MRB_PROC_ENV(m)->mid;
      }

      /* prepare stack */
      if (MRB_PROC_CFUNC_P(m)) {
        recv = MRB_PROC_CFUNC(m)(mrb, recv);
        mrb_gc_arena_shrink(mrb, ai);
        if (mrb->exc) goto L_RAISE;
        /* pop stackpos */
        ci = cipop(mrb);
        pc = ci->pc;
        ci[1].stack[0] = recv;
        irep = mrb->c->ci->proc->body.irep;
      }
      else {
        /* setup environment for calling method */
        proc = m;
        irep = m->body.irep;
        if (!irep) {
          mrb->c->ci->stack[0] = mrb_nil_value();
          a = 0;
          c = OP_R_NORMAL;
          goto L_OP_RETURN_BODY;
        }
        mrb_int nargs = mrb_ci_bidx(ci)+1;
        if (nargs < irep->nregs) {
          mrb_stack_extend(mrb, irep->nregs);
          stack_clear(regs+nargs, irep->nregs-nargs);
        }
        if (MRB_PROC_ENV_P(m)) {
          regs[0] = MRB_PROC_ENV(m)->stack[0];
        }
        pc = irep->iseq;
      }
      pool = irep->pool;
      syms = irep->syms;
      JUMP;
    }

    CASE(OP_SUPER, BB) {
      mrb_method_t m;
      struct RClass *cls;
      mrb_callinfo *ci = mrb->c->ci;
      mrb_value recv, blk;
      const struct RProc *p = ci->proc;
      mrb_sym mid = ci->mid;
      struct RClass* target_class = MRB_PROC_TARGET_CLASS(p);

      if (MRB_PROC_ENV_P(p) && p->e.env->mid && p->e.env->mid != mid) { /* alias support */
        mid = p->e.env->mid;    /* restore old mid */
      }

      if (mid == 0 || !target_class) {
        mrb_value exc = mrb_exc_new_lit(mrb, E_NOMETHOD_ERROR, "super called outside of method");
        mrb_exc_set(mrb, exc);
        goto L_RAISE;
      }
      if (target_class->flags & MRB_FL_CLASS_IS_PREPENDED) {
        target_class = mrb_vm_ci_target_class(ci);
      }
      else if (target_class->tt == MRB_TT_MODULE) {
        target_class = mrb_vm_ci_target_class(ci);
        if (!target_class || target_class->tt != MRB_TT_ICLASS) {
          goto super_typeerror;
        }
      }
      recv = regs[0];
      if (!mrb_obj_is_kind_of(mrb, recv, target_class)) {
      super_typeerror: ;
        mrb_value exc = mrb_exc_new_lit(mrb, E_TYPE_ERROR,
                                            "self has wrong type to call super in this context");
        mrb_exc_set(mrb, exc);
        goto L_RAISE;
      }

      ARGUMENT_NORMALIZE(a, &b, OP_SUPER);

      cls = target_class->super;
      m = mrb_method_search_vm(mrb, &cls, mid);
      if (MRB_METHOD_UNDEF_P(m)) {
        m = prepare_missing(mrb, recv, mid, &cls, a, &b, blk, 1);
        mid = MRB_SYM(method_missing);
      }

      /* push callinfo */
      ci = cipush(mrb, a, 0, cls, NULL, mid, b);

      /* prepare stack */
      ci->stack[0] = recv;

      if (MRB_METHOD_CFUNC_P(m)) {
        mrb_value v;

        if (MRB_METHOD_PROC_P(m)) {
          mrb_vm_ci_proc_set(ci, MRB_METHOD_PROC(m));
        }
        v = MRB_METHOD_CFUNC(m)(mrb, recv);
        mrb_gc_arena_restore(mrb, ai);
        if (mrb->exc) goto L_RAISE;
        ci = mrb->c->ci;
        mrb_assert(!mrb_break_p(v));
        if (!mrb_vm_ci_target_class(ci)) { /* return from context modifying method (resume/yield) */
          if (ci->cci == CINFO_RESUMED) {
            mrb->jmp = prev_jmp;
            return v;
          }
          else {
            mrb_assert(!MRB_PROC_CFUNC_P(ci[-1].proc));
            proc = ci[-1].proc;
            irep = proc->body.irep;
            pool = irep->pool;
            syms = irep->syms;
          }
        }
        mrb->c->ci->stack[0] = v;
        ci = cipop(mrb);
        pc = ci->pc;
      }
      else {
        /* setup environment for calling method */
        mrb_vm_ci_proc_set(ci, (proc = MRB_METHOD_PROC(m)));
        irep = proc->body.irep;
        pool = irep->pool;
        syms = irep->syms;
        mrb_stack_extend(mrb, (irep->nregs < 4) ? 4 : irep->nregs);
        pc = irep->iseq;
      }
      JUMP;
    }

    CASE(OP_ARGARY, BS) {
      mrb_int m1 = (b>>11)&0x3f;
      mrb_int r  = (b>>10)&0x1;
      mrb_int m2 = (b>>5)&0x1f;
      mrb_int kd = (b>>4)&0x1;
      mrb_int lv = (b>>0)&0xf;
      mrb_value *stack;

      if (mrb->c->ci->mid == 0 || mrb_vm_ci_target_class(mrb->c->ci) == NULL) {
        mrb_value exc;

      L_NOSUPER:
        exc = mrb_exc_new_lit(mrb, E_NOMETHOD_ERROR, "super called outside of method");
        mrb_exc_set(mrb, exc);
        goto L_RAISE;
      }
      if (lv == 0) stack = regs + 1;
      else {
        struct REnv *e = uvenv(mrb, lv-1);
        if (!e) goto L_NOSUPER;
        if (MRB_ENV_LEN(e) <= m1+r+m2+1)
          goto L_NOSUPER;
        stack = e->stack + 1;
      }
      if (r == 0) {
        regs[a] = mrb_ary_new_from_values(mrb, m1+m2, stack);
      }
      else {
        mrb_value *pp = NULL;
        struct RArray *rest;
        mrb_int len = 0;

        if (mrb_array_p(stack[m1])) {
          struct RArray *ary = mrb_ary_ptr(stack[m1]);

          pp = ARY_PTR(ary);
          len = ARY_LEN(ary);
        }
        regs[a] = mrb_ary_new_capa(mrb, m1+len+m2);
        rest = mrb_ary_ptr(regs[a]);
        if (m1 > 0) {
          stack_copy(ARY_PTR(rest), stack, m1);
        }
        if (len > 0) {
          stack_copy(ARY_PTR(rest)+m1, pp, len);
        }
        if (m2 > 0) {
          stack_copy(ARY_PTR(rest)+m1+len, stack+m1+1, m2);
        }
        ARY_SET_LEN(rest, m1+len+m2);
      }
      if (kd) {
        regs[a+1] = stack[m1+r+m2];
        regs[a+2] = stack[m1+r+m2+1];
      }
      else {
        regs[a+1] = stack[m1+r+m2];
      }
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_ENTER, W) {
      mrb_int m1 = MRB_ASPEC_REQ(a);
      mrb_int o  = MRB_ASPEC_OPT(a);
      mrb_int r  = MRB_ASPEC_REST(a);
      mrb_int m2 = MRB_ASPEC_POST(a);
      mrb_int kd = (MRB_ASPEC_KEY(a) > 0 || MRB_ASPEC_KDICT(a))? 1 : 0;
      /* unused
      int b  = MRB_ASPEC_BLOCK(a);
      */
      mrb_int const len = m1 + o + r + m2;

      mrb_callinfo *ci = mrb->c->ci;
      mrb_int argc = ci->n;
      mrb_value *argv = regs+1;
      mrb_value * const argv0 = argv;
      mrb_int const kw_pos = len + kd;    /* where kwhash should be */
      mrb_int const blk_pos = kw_pos + 1; /* where block should be */
      mrb_value blk = regs[mrb_ci_bidx(ci)];
      mrb_value kdict = mrb_nil_value();

      /* keyword arguments */
      if (ci->nk > 0) {
        mrb_int kidx = mrb_ci_kidx(ci);
        kdict = regs[kidx];
        if (!mrb_hash_p(kdict) || mrb_hash_size(mrb, kdict) == 0) {
          kdict = mrb_nil_value();
          ci->nk = 0;
        }
      }
      if (!kd && !mrb_nil_p(kdict)) {
        if (argc < 14) {
          ci->n++;
          argc++;    /* include kdict in normal arguments */
        }
        else if (argc == 14) {
          /* pack arguments and kdict */
          regs[1] = mrb_ary_new_from_values(mrb, argc+1, &regs[1]);
          argc = ci->n = 15;
        }
        else {/* argc == 15 */
          /* push kdict to packed arguments */
          mrb_ary_push(mrb, regs[1], regs[2]);
        }
        ci->nk = 0;
      }
      if (kd && MRB_ASPEC_KEY(a) > 0 && mrb_hash_p(kdict)) {
        kdict = mrb_hash_dup(mrb, kdict);
      }

      /* arguments is passed with Array */
      if (argc == 15) {
        struct RArray *ary = mrb_ary_ptr(regs[1]);
        argv = ARY_PTR(ary);
        argc = (int)ARY_LEN(ary);
        mrb_gc_protect(mrb, regs[1]);
      }

      /* strict argument check */
      if (ci->proc && MRB_PROC_STRICT_P(ci->proc)) {
        if (argc < m1 + m2 || (r == 0 && argc > len)) {
          argnum_error(mrb, m1+m2);
          goto L_RAISE;
        }
      }
      /* extract first argument array to arguments */
      else if (len > 1 && argc == 1 && mrb_array_p(argv[0])) {
        mrb_gc_protect(mrb, argv[0]);
        argc = (int)RARRAY_LEN(argv[0]);
        argv = RARRAY_PTR(argv[0]);
      }

      /* rest arguments */
      mrb_value rest = mrb_nil_value();
      if (argc < len) {
        mrb_int mlen = m2;
        if (argc < m1+m2) {
          mlen = m1 < argc ? argc - m1 : 0;
        }

        /* copy mandatory and optional arguments */
        if (argv0 != argv && argv) {
          value_move(&regs[1], argv, argc-mlen); /* m1 + o */
        }
        if (argc < m1) {
          stack_clear(&regs[argc+1], m1-argc);
        }
        /* copy post mandatory arguments */
        if (mlen) {
          value_move(&regs[len-m2+1], &argv[argc-mlen], mlen);
        }
        if (mlen < m2) {
          stack_clear(&regs[len-m2+mlen+1], m2-mlen);
        }
        /* initialize rest arguments with empty Array */
        if (r) {
          rest = mrb_ary_new_capa(mrb, 0);
          regs[m1+o+1] = rest;
        }
        /* skip initializer of passed arguments */
        if (o > 0 && argc > m1+m2)
          pc += (argc - m1 - m2)*3;
      }
      else {
        mrb_int rnum = 0;
        if (argv0 != argv) {
          value_move(&regs[1], argv, m1+o);
        }
        if (r) {
          rnum = argc-m1-o-m2;
          rest = mrb_ary_new_from_values(mrb, rnum, argv+m1+o);
          regs[m1+o+1] = rest;
        }
        if (m2 > 0 && argc-m2 > m1) {
          value_move(&regs[m1+o+r+1], &argv[m1+o+rnum], m2);
        }
        pc += o*3;
      }

      /* need to be update blk first to protect blk from GC */
      regs[blk_pos] = blk;              /* move block */
      if (kd) {
        if (mrb_nil_p(kdict))
          kdict = mrb_hash_new_capa(mrb, 0);
        regs[kw_pos] = kdict;           /* set kwhash */
      }

      /* format arguments for generated code */
      mrb->c->ci->n = len;

      /* clear local (but non-argument) variables */
      if (irep->nlocals-blk_pos-1 > 0) {
        stack_clear(&regs[blk_pos+1], irep->nlocals-blk_pos-1);
      }
      JUMP;
    }

    CASE(OP_KARG, BB) {
      mrb_value k = mrb_symbol_value(syms[b]);
      mrb_int kidx = mrb_ci_kidx(mrb->c->ci);
      mrb_value kdict, v;

      if (kidx < 0 || !mrb_hash_p(kdict=regs[kidx]) || !mrb_hash_key_p(mrb, kdict, k)) {
        mrb_value str = mrb_format(mrb, "missing keyword: %v", k);
        mrb_exc_set(mrb, mrb_exc_new_str(mrb, E_ARGUMENT_ERROR, str));
        goto L_RAISE;
      }
      v = mrb_hash_get(mrb, kdict, k);
      regs[a] = v;
      mrb_hash_delete_key(mrb, kdict, k);
      NEXT;
    }

    CASE(OP_KEY_P, BB) {
      mrb_value k = mrb_symbol_value(syms[b]);
      mrb_int kidx = mrb_ci_kidx(mrb->c->ci);
      mrb_value kdict;
      mrb_bool key_p = FALSE;

      if (kidx >= 0 && mrb_hash_p(kdict=regs[kidx])) {
        key_p = mrb_hash_key_p(mrb, kdict, k);
      }
      regs[a] = mrb_bool_value(key_p);
      NEXT;
    }

    CASE(OP_KEYEND, Z) {
      mrb_int kidx = mrb_ci_kidx(mrb->c->ci);
      mrb_value kdict;

      if (kidx >= 0 && mrb_hash_p(kdict=regs[kidx]) && !mrb_hash_empty_p(mrb, kdict)) {
        mrb_value keys = mrb_hash_keys(mrb, kdict);
        mrb_value key1 = RARRAY_PTR(keys)[0];
        mrb_value str = mrb_format(mrb, "unknown keyword: %v", key1);
        mrb_exc_set(mrb, mrb_exc_new_str(mrb, E_ARGUMENT_ERROR, str));
        goto L_RAISE;
      }
      NEXT;
    }

    CASE(OP_BREAK, B) {
      c = OP_R_BREAK;
      goto L_RETURN;
    }
    CASE(OP_RETURN_BLK, B) {
      c = OP_R_RETURN;
      goto L_RETURN;
    }
    CASE(OP_RETURN, B)
    c = OP_R_NORMAL;
    L_RETURN:
    {
      mrb_callinfo *ci;

      ci = mrb->c->ci;
      if (ci->mid) {
        mrb_value blk = regs[mrb_ci_bidx(ci)];

        if (mrb_proc_p(blk)) {
          struct RProc *p = mrb_proc_ptr(blk);

          if (!MRB_PROC_STRICT_P(p) &&
              ci > mrb->c->cibase && MRB_PROC_ENV(p) == mrb_vm_ci_env(&ci[-1])) {
            p->flags |= MRB_PROC_ORPHAN;
          }
        }
      }

      if (mrb->exc) {
      L_RAISE:
        ci = mrb->c->ci;
        if (ci == mrb->c->cibase) {
          ch = catch_handler_find(mrb, ci, pc, MRB_CATCH_FILTER_ALL);
          if (ch == NULL) goto L_FTOP;
          goto L_CATCH;
        }
        while ((ch = catch_handler_find(mrb, ci, pc, MRB_CATCH_FILTER_ALL)) == NULL) {
          ci = cipop(mrb);
          if (ci[1].cci == CINFO_SKIP && prev_jmp) {
            mrb->jmp = prev_jmp;
            MRB_THROW(prev_jmp);
          }
          pc = ci[0].pc;
          if (ci == mrb->c->cibase) {
            ch = catch_handler_find(mrb, ci, pc, MRB_CATCH_FILTER_ALL);
            if (ch == NULL) {
            L_FTOP:             /* fiber top */
              if (mrb->c == mrb->root_c) {
                mrb->c->ci->stack = mrb->c->stbase;
                goto L_STOP;
              }
              else {
                struct mrb_context *c = mrb->c;

                c->status = MRB_FIBER_TERMINATED;
                mrb->c = c->prev;
                c->prev = NULL;
                goto L_RAISE;
              }
            }
            break;
          }
        }
      L_CATCH:
        if (ch == NULL) goto L_STOP;
        if (FALSE) {
        L_CATCH_TAGGED_BREAK: /* from THROW_TAGGED_BREAK() or UNWIND_ENSURE() */
          ci = mrb->c->ci;
        }
        proc = ci->proc;
        irep = proc->body.irep;
        pool = irep->pool;
        syms = irep->syms;
        mrb_stack_extend(mrb, irep->nregs);
        pc = irep->iseq + mrb_irep_catch_handler_unpack(ch->target);
      }
      else {
        mrb_int acc;
        mrb_value v;

        ci = mrb->c->ci;
        v = regs[a];
        mrb_gc_protect(mrb, v);
        switch (c) {
        case OP_R_RETURN:
          /* Fall through to OP_R_NORMAL otherwise */
          if (ci->cci == CINFO_NONE && MRB_PROC_ENV_P(proc) && !MRB_PROC_STRICT_P(proc)) {
            const struct RProc *dst;
            mrb_callinfo *cibase;
            cibase = mrb->c->cibase;
            dst = top_proc(mrb, proc);

            if (MRB_PROC_ENV_P(dst)) {
              struct REnv *e = MRB_PROC_ENV(dst);

              if (!MRB_ENV_ONSTACK_P(e) || (e->cxt && e->cxt != mrb->c)) {
                localjump_error(mrb, LOCALJUMP_ERROR_RETURN);
                goto L_RAISE;
              }
            }
            /* check jump destination */
            while (cibase <= ci && ci->proc != dst) {
              if (ci->cci > CINFO_NONE) { /* jump cross C boundary */
                localjump_error(mrb, LOCALJUMP_ERROR_RETURN);
                goto L_RAISE;
              }
              ci--;
            }
            if (ci <= cibase) { /* no jump destination */
              localjump_error(mrb, LOCALJUMP_ERROR_RETURN);
              goto L_RAISE;
            }
            ci = mrb->c->ci;
            while (cibase <= ci && ci->proc != dst) {
              CHECKPOINT_RESTORE(RBREAK_TAG_RETURN_BLOCK) {
                cibase = mrb->c->cibase;
                dst = top_proc(mrb, proc);
              }
              CHECKPOINT_MAIN(RBREAK_TAG_RETURN_BLOCK) {
                UNWIND_ENSURE(mrb, ci, pc, RBREAK_TAG_RETURN_BLOCK, proc, v);
              }
              CHECKPOINT_END(RBREAK_TAG_RETURN_BLOCK);
              ci = cipop(mrb);
              pc = ci->pc;
            }
            proc = ci->proc;
            mrb->exc = NULL; /* clear break object */
            break;
          }
          /* fallthrough */
        case OP_R_NORMAL:
        NORMAL_RETURN:
          if (ci == mrb->c->cibase) {
            struct mrb_context *c;
            c = mrb->c;

            if (!c->prev) { /* toplevel return */
              regs[irep->nlocals] = v;
              goto CHECKPOINT_LABEL_MAKE(RBREAK_TAG_STOP);
            }
            if (!c->vmexec && c->prev->ci == c->prev->cibase) {
              mrb_value exc = mrb_exc_new_lit(mrb, E_FIBER_ERROR, "double resume");
              mrb_exc_set(mrb, exc);
              goto L_RAISE;
            }
            CHECKPOINT_RESTORE(RBREAK_TAG_RETURN_TOPLEVEL) {
              c = mrb->c;
            }
            CHECKPOINT_MAIN(RBREAK_TAG_RETURN_TOPLEVEL) {
              UNWIND_ENSURE(mrb, ci, pc, RBREAK_TAG_RETURN_TOPLEVEL, proc, v);
            }
            CHECKPOINT_END(RBREAK_TAG_RETURN_TOPLEVEL);
            /* automatic yield at the end */
            c->status = MRB_FIBER_TERMINATED;
            mrb->c = c->prev;
            mrb->c->status = MRB_FIBER_RUNNING;
            c->prev = NULL;
            if (c->vmexec) {
              mrb_gc_arena_restore(mrb, ai);
              c->vmexec = FALSE;
              mrb->jmp = prev_jmp;
              return v;
            }
            ci = mrb->c->ci;
          }
          CHECKPOINT_RESTORE(RBREAK_TAG_RETURN) {
            /* do nothing */
          }
          CHECKPOINT_MAIN(RBREAK_TAG_RETURN) {
            UNWIND_ENSURE(mrb, ci, pc, RBREAK_TAG_RETURN, proc, v);
          }
          CHECKPOINT_END(RBREAK_TAG_RETURN);
          mrb->exc = NULL; /* clear break object */
          break;
        case OP_R_BREAK:
          if (MRB_PROC_STRICT_P(proc)) goto NORMAL_RETURN;
          if (MRB_PROC_ORPHAN_P(proc)) {
            mrb_value exc;

          L_BREAK_ERROR:
            exc = mrb_exc_new_lit(mrb, E_LOCALJUMP_ERROR,
                                      "break from proc-closure");
            mrb_exc_set(mrb, exc);
            goto L_RAISE;
          }
          if (!MRB_PROC_ENV_P(proc) || !MRB_ENV_ONSTACK_P(MRB_PROC_ENV(proc))) {
            goto L_BREAK_ERROR;
          }
          else {
            struct REnv *e = MRB_PROC_ENV(proc);

            if (e->cxt != mrb->c) {
              goto L_BREAK_ERROR;
            }
          }
          CHECKPOINT_RESTORE(RBREAK_TAG_BREAK) {
            /* do nothing */
          }
          CHECKPOINT_MAIN(RBREAK_TAG_BREAK) {
            UNWIND_ENSURE(mrb, ci, pc, RBREAK_TAG_BREAK, proc, v);
          }
          CHECKPOINT_END(RBREAK_TAG_BREAK);
          /* break from fiber block */
          if (ci == mrb->c->cibase && ci->pc) {
            struct mrb_context *c = mrb->c;

            mrb->c = c->prev;
            c->prev = NULL;
            ci = mrb->c->ci;
          }
          if (ci->cci > CINFO_NONE) {
            ci = cipop(mrb);
            mrb_gc_arena_restore(mrb, ai);
            mrb->c->vmexec = FALSE;
            mrb->exc = (struct RObject*)break_new(mrb, RBREAK_TAG_BREAK, proc, v);
            mrb->jmp = prev_jmp;
            MRB_THROW(prev_jmp);
          }
          if (FALSE) {
            struct RBreak *brk;

          L_BREAK:
            brk = (struct RBreak*)mrb->exc;
            proc = mrb_break_proc_get(brk);
            v = mrb_break_value_get(brk);
            ci = mrb->c->ci;

            switch (mrb_break_tag_get(brk)) {
#define DISPATCH_CHECKPOINTS(n, i) case n: goto CHECKPOINT_LABEL_MAKE(n);
              RBREAK_TAG_FOREACH(DISPATCH_CHECKPOINTS)
#undef DISPATCH_CHECKPOINTS
              default:
                mrb_assert(!"wrong break tag");
            }
          }
          while (mrb->c->cibase < ci && ci[-1].proc != proc->upper) {
            if (ci[-1].cci == CINFO_SKIP) {
              goto L_BREAK_ERROR;
            }
            CHECKPOINT_RESTORE(RBREAK_TAG_BREAK_UPPER) {
              /* do nothing */
            }
            CHECKPOINT_MAIN(RBREAK_TAG_BREAK_UPPER) {
              UNWIND_ENSURE(mrb, ci, pc, RBREAK_TAG_BREAK_UPPER, proc, v);
            }
            CHECKPOINT_END(RBREAK_TAG_BREAK_UPPER);
            ci = cipop(mrb);
            pc = ci->pc;
          }
          CHECKPOINT_RESTORE(RBREAK_TAG_BREAK_INTARGET) {
            /* do nothing */
          }
          CHECKPOINT_MAIN(RBREAK_TAG_BREAK_INTARGET) {
            UNWIND_ENSURE(mrb, ci, pc, RBREAK_TAG_BREAK_INTARGET, proc, v);
          }
          CHECKPOINT_END(RBREAK_TAG_BREAK_INTARGET);
          if (ci == mrb->c->cibase) {
            goto L_BREAK_ERROR;
          }
          mrb->exc = NULL; /* clear break object */
          break;
        default:
          /* cannot happen */
          break;
        }
        mrb_assert(ci == mrb->c->ci);
        mrb_assert(mrb->exc == NULL);

        if (mrb->c->vmexec && !mrb_vm_ci_target_class(ci)) {
          mrb_gc_arena_restore(mrb, ai);
          mrb->c->vmexec = FALSE;
          mrb->jmp = prev_jmp;
          return v;
        }
        acc = ci->cci;
        ci = cipop(mrb);
        if (acc == CINFO_SKIP || acc == CINFO_DIRECT) {
          mrb_gc_arena_restore(mrb, ai);
          mrb->jmp = prev_jmp;
          return v;
        }
        pc = ci->pc;
        DEBUG(fprintf(stderr, "from :%s\n", mrb_sym_name(mrb, ci->mid)));
        proc = ci->proc;
        irep = proc->body.irep;
        pool = irep->pool;
        syms = irep->syms;

        ci[1].stack[0] = v;
        mrb_gc_arena_restore(mrb, ai);
      }
      JUMP;
    }

    CASE(OP_BLKPUSH, BS) {
      int m1 = (b>>11)&0x3f;
      int r  = (b>>10)&0x1;
      int m2 = (b>>5)&0x1f;
      int kd = (b>>4)&0x1;
      int lv = (b>>0)&0xf;
      mrb_value *stack;

      if (lv == 0) stack = regs + 1;
      else {
        struct REnv *e = uvenv(mrb, lv-1);
        if (!e || (!MRB_ENV_ONSTACK_P(e) && e->mid == 0) ||
            MRB_ENV_LEN(e) <= m1+r+m2+1) {
          localjump_error(mrb, LOCALJUMP_ERROR_YIELD);
          goto L_RAISE;
        }
        stack = e->stack + 1;
      }
      if (mrb_nil_p(stack[m1+r+m2+kd])) {
        localjump_error(mrb, LOCALJUMP_ERROR_YIELD);
        goto L_RAISE;
      }
      regs[a] = stack[m1+r+m2+kd];
      NEXT;
    }

  L_INT_OVERFLOW:
    {
      mrb_value exc = mrb_exc_new_lit(mrb, E_RANGE_ERROR, "integer overflow");
      mrb_exc_set(mrb, exc);
    }
    goto L_RAISE;

#define TYPES2(a,b) ((((uint16_t)(a))<<8)|(((uint16_t)(b))&0xff))
#define OP_MATH(op_name)                                                    \
  /* need to check if op is overridden */                                   \
  switch (TYPES2(mrb_type(regs[a]),mrb_type(regs[a+1]))) {                  \
    OP_MATH_CASE_INTEGER(op_name);                                          \
    OP_MATH_CASE_FLOAT(op_name, integer, float);                            \
    OP_MATH_CASE_FLOAT(op_name, float,  integer);                           \
    OP_MATH_CASE_FLOAT(op_name, float,  float);                             \
    OP_MATH_CASE_STRING_##op_name();                                        \
    default:                                                                \
      mid = MRB_OPSYM(op_name);                                             \
      goto L_SEND_SYM;                                                      \
  }                                                                         \
  NEXT;
#define OP_MATH_CASE_INTEGER(op_name)                                       \
  case TYPES2(MRB_TT_INTEGER, MRB_TT_INTEGER):                              \
    {                                                                       \
      mrb_int x = mrb_integer(regs[a]), y = mrb_integer(regs[a+1]), z;      \
      if (mrb_int_##op_name##_overflow(x, y, &z))                           \
        OP_MATH_OVERFLOW_INT();                                             \
      else                                                                  \
        SET_INT_VALUE(mrb,regs[a], z);                                      \
    }                                                                       \
    break
#ifdef MRB_NO_FLOAT
#define OP_MATH_CASE_FLOAT(op_name, t1, t2) (void)0
#else
#define OP_MATH_CASE_FLOAT(op_name, t1, t2)                                     \
  case TYPES2(OP_MATH_TT_##t1, OP_MATH_TT_##t2):                                \
    {                                                                           \
      mrb_float z = mrb_##t1(regs[a]) OP_MATH_OP_##op_name mrb_##t2(regs[a+1]); \
      SET_FLOAT_VALUE(mrb, regs[a], z);                                         \
    }                                                                           \
    break
#endif
#define OP_MATH_OVERFLOW_INT() goto L_INT_OVERFLOW
#define OP_MATH_CASE_STRING_add()                                           \
  case TYPES2(MRB_TT_STRING, MRB_TT_STRING):                                \
    regs[a] = mrb_str_plus(mrb, regs[a], regs[a+1]);                        \
    mrb_gc_arena_restore(mrb, ai);                                          \
    break
#define OP_MATH_CASE_STRING_sub() (void)0
#define OP_MATH_CASE_STRING_mul() (void)0
#define OP_MATH_OP_add +
#define OP_MATH_OP_sub -
#define OP_MATH_OP_mul *
#define OP_MATH_TT_integer MRB_TT_INTEGER
#define OP_MATH_TT_float   MRB_TT_FLOAT

    CASE(OP_ADD, B) {
      OP_MATH(add);
    }

    CASE(OP_SUB, B) {
      OP_MATH(sub);
    }

    CASE(OP_MUL, B) {
      OP_MATH(mul);
    }

    CASE(OP_DIV, B) {
#ifndef MRB_NO_FLOAT
      mrb_float x, y, f;
#endif

      /* need to check if op is overridden */
      switch (TYPES2(mrb_type(regs[a]),mrb_type(regs[a+1]))) {
      case TYPES2(MRB_TT_INTEGER,MRB_TT_INTEGER):
        {
          mrb_int x = mrb_integer(regs[a]);
          mrb_int y = mrb_integer(regs[a+1]);
          mrb_int div = mrb_div_int(mrb, x, y);
          SET_INT_VALUE(mrb, regs[a], div);
        }
        NEXT;
#ifndef MRB_NO_FLOAT
      case TYPES2(MRB_TT_INTEGER,MRB_TT_FLOAT):
        x = (mrb_float)mrb_integer(regs[a]);
        y = mrb_float(regs[a+1]);
        break;
      case TYPES2(MRB_TT_FLOAT,MRB_TT_INTEGER):
        x = mrb_float(regs[a]);
        y = (mrb_float)mrb_integer(regs[a+1]);
        break;
      case TYPES2(MRB_TT_FLOAT,MRB_TT_FLOAT):
        x = mrb_float(regs[a]);
        y = mrb_float(regs[a+1]);
        break;
#endif
      default:
        mid = MRB_OPSYM(div);
        goto L_SEND_SYM;
      }

#ifndef MRB_NO_FLOAT
      f = mrb_div_float(x, y);
      SET_FLOAT_VALUE(mrb, regs[a], f);
#endif
      NEXT;
    }

#define OP_MATHI(op_name)                                                   \
  /* need to check if op is overridden */                                   \
  switch (mrb_type(regs[a])) {                                              \
    OP_MATHI_CASE_INTEGER(op_name);                                         \
    OP_MATHI_CASE_FLOAT(op_name);                                           \
    default:                                                                \
      SET_INT_VALUE(mrb,regs[a+1], b);                                      \
      mid = MRB_OPSYM(op_name);                                             \
      goto L_SEND_SYM;                                                      \
  }                                                                         \
  NEXT;
#define OP_MATHI_CASE_INTEGER(op_name)                                      \
  case MRB_TT_INTEGER:                                                      \
    {                                                                       \
      mrb_int x = mrb_integer(regs[a]), y = (mrb_int)b, z;                  \
      if (mrb_int_##op_name##_overflow(x, y, &z))                           \
        OP_MATH_OVERFLOW_INT();                                             \
      else                                                                  \
        SET_INT_VALUE(mrb,regs[a], z);                                      \
    }                                                                       \
    break
#ifdef MRB_NO_FLOAT
#define OP_MATHI_CASE_FLOAT(op_name) (void)0
#else
#define OP_MATHI_CASE_FLOAT(op_name)                                        \
  case MRB_TT_FLOAT:                                                        \
    {                                                                       \
      mrb_float z = mrb_float(regs[a]) OP_MATH_OP_##op_name b;              \
      SET_FLOAT_VALUE(mrb, regs[a], z);                                     \
    }                                                                       \
    break
#endif

    CASE(OP_ADDI, BB) {
      OP_MATHI(add);
    }

    CASE(OP_SUBI, BB) {
      OP_MATHI(sub);
    }

#define OP_CMP_BODY(op,v1,v2) (v1(regs[a]) op v2(regs[a+1]))

#ifdef MRB_NO_FLOAT
#define OP_CMP(op,sym) do {\
  int result;\
  /* need to check if - is overridden */\
  switch (TYPES2(mrb_type(regs[a]),mrb_type(regs[a+1]))) {\
  case TYPES2(MRB_TT_INTEGER,MRB_TT_INTEGER):\
    result = OP_CMP_BODY(op,mrb_fixnum,mrb_fixnum);\
    break;\
  default:\
    mid = MRB_OPSYM(sym);\
    goto L_SEND_SYM;\
  }\
  if (result) {\
    SET_TRUE_VALUE(regs[a]);\
  }\
  else {\
    SET_FALSE_VALUE(regs[a]);\
  }\
} while(0)
#else
#define OP_CMP(op, sym) do {\
  int result;\
  /* need to check if - is overridden */\
  switch (TYPES2(mrb_type(regs[a]),mrb_type(regs[a+1]))) {\
  case TYPES2(MRB_TT_INTEGER,MRB_TT_INTEGER):\
    result = OP_CMP_BODY(op,mrb_fixnum,mrb_fixnum);\
    break;\
  case TYPES2(MRB_TT_INTEGER,MRB_TT_FLOAT):\
    result = OP_CMP_BODY(op,mrb_fixnum,mrb_float);\
    break;\
  case TYPES2(MRB_TT_FLOAT,MRB_TT_INTEGER):\
    result = OP_CMP_BODY(op,mrb_float,mrb_fixnum);\
    break;\
  case TYPES2(MRB_TT_FLOAT,MRB_TT_FLOAT):\
    result = OP_CMP_BODY(op,mrb_float,mrb_float);\
    break;\
  default:\
    mid = MRB_OPSYM(sym);\
    goto L_SEND_SYM;\
  }\
  if (result) {\
    SET_TRUE_VALUE(regs[a]);\
  }\
  else {\
    SET_FALSE_VALUE(regs[a]);\
  }\
} while(0)
#endif

    CASE(OP_EQ, B) {
      if (mrb_obj_eq(mrb, regs[a], regs[a+1])) {
        SET_TRUE_VALUE(regs[a]);
      }
      else {
        OP_CMP(==,eq);
      }
      NEXT;
    }

    CASE(OP_LT, B) {
      OP_CMP(<,lt);
      NEXT;
    }

    CASE(OP_LE, B) {
      OP_CMP(<=,le);
      NEXT;
    }

    CASE(OP_GT, B) {
      OP_CMP(>,gt);
      NEXT;
    }

    CASE(OP_GE, B) {
      OP_CMP(>=,ge);
      NEXT;
    }

    CASE(OP_ARRAY, BB) {
      regs[a] = mrb_ary_new_from_values(mrb, b, &regs[a]);
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }
    CASE(OP_ARRAY2, BBB) {
      regs[a] = mrb_ary_new_from_values(mrb, c, &regs[b]);
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_ARYCAT, B) {
      mrb_value splat = mrb_ary_splat(mrb, regs[a+1]);
      if (mrb_nil_p(regs[a])) {
        regs[a] = splat;
      }
      else {
        mrb_assert(mrb_array_p(regs[a]));
        mrb_ary_concat(mrb, regs[a], splat);
      }
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_ARYPUSH, BB) {
      mrb_assert(mrb_array_p(regs[a]));
      for (mrb_int i=0; i<b; i++) {
        mrb_ary_push(mrb, regs[a], regs[a+i+1]);
      }
      NEXT;
    }

    CASE(OP_ARYDUP, B) {
      mrb_value ary = regs[a];
      if (mrb_array_p(ary)) {
        ary = mrb_ary_new_from_values(mrb, RARRAY_LEN(ary), RARRAY_PTR(ary));
      }
      else {
        ary = mrb_ary_new_from_values(mrb, 1, &ary);
      }
      regs[a] = ary;
      NEXT;
    }

    CASE(OP_AREF, BBB) {
      mrb_value v = regs[b];

      if (!mrb_array_p(v)) {
        if (c == 0) {
          regs[a] = v;
        }
        else {
          SET_NIL_VALUE(regs[a]);
        }
      }
      else {
        v = mrb_ary_ref(mrb, v, c);
        regs[a] = v;
      }
      NEXT;
    }

    CASE(OP_ASET, BBB) {
      mrb_assert(mrb_array_p(regs[a]));
      mrb_ary_set(mrb, regs[b], c, regs[a]);
      NEXT;
    }

    CASE(OP_APOST, BBB) {
      mrb_value v = regs[a];
      int pre  = b;
      int post = c;
      struct RArray *ary;
      int len, idx;

      if (!mrb_array_p(v)) {
        v = mrb_ary_new_from_values(mrb, 1, &regs[a]);
      }
      ary = mrb_ary_ptr(v);
      len = (int)ARY_LEN(ary);
      if (len > pre + post) {
        v = mrb_ary_new_from_values(mrb, len - pre - post, ARY_PTR(ary)+pre);
        regs[a++] = v;
        while (post--) {
          regs[a++] = ARY_PTR(ary)[len-post-1];
        }
      }
      else {
        v = mrb_ary_new_capa(mrb, 0);
        regs[a++] = v;
        for (idx=0; idx+pre<len; idx++) {
          regs[a+idx] = ARY_PTR(ary)[pre+idx];
        }
        while (idx < post) {
          SET_NIL_VALUE(regs[a+idx]);
          idx++;
        }
      }
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_INTERN, B) {
      mrb_assert(mrb_string_p(regs[a]));
      mrb_sym sym = mrb_intern_str(mrb, regs[a]);
      regs[a] = mrb_symbol_value(sym);
      NEXT;
    }

    CASE(OP_SYMBOL, BB) {
      size_t len;
      mrb_sym sym;

      mrb_assert((pool[b].tt&IREP_TT_NFLAG)==0);
      len = pool[b].tt >> 2;
      if (pool[b].tt & IREP_TT_SFLAG) {
        sym = mrb_intern_static(mrb, pool[b].u.str, len);
      }
      else {
        sym  = mrb_intern(mrb, pool[b].u.str, len);
      }
      regs[a] = mrb_symbol_value(sym);
      NEXT;
    }

    CASE(OP_STRING, BB) {
      mrb_int len;

      mrb_assert((pool[b].tt&IREP_TT_NFLAG)==0);
      len = pool[b].tt >> 2;
      if (pool[b].tt & IREP_TT_SFLAG) {
        regs[a] = mrb_str_new_static(mrb, pool[b].u.str, len);
      }
      else {
        regs[a] = mrb_str_new(mrb, pool[b].u.str, len);
      }
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_STRCAT, B) {
      mrb_assert(mrb_string_p(regs[a]));
      mrb_str_concat(mrb, regs[a], regs[a+1]);
      NEXT;
    }

    CASE(OP_HASH, BB) {
      mrb_value hash = mrb_hash_new_capa(mrb, b);
      int i;
      int lim = a+b*2;

      for (i=a; i<lim; i+=2) {
        mrb_hash_set(mrb, hash, regs[i], regs[i+1]);
      }
      regs[a] = hash;
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_HASHADD, BB) {
      mrb_value hash;
      int i;
      int lim = a+b*2+1;

      hash = regs[a];
      mrb_ensure_hash_type(mrb, hash);
      for (i=a+1; i<lim; i+=2) {
        mrb_hash_set(mrb, hash, regs[i], regs[i+1]);
      }
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }
    CASE(OP_HASHCAT, B) {
      mrb_value hash = regs[a];

      mrb_assert(mrb_hash_p(hash));
      mrb_hash_merge(mrb, hash, regs[a+1]);
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_LAMBDA, BB)
    c = OP_L_LAMBDA;
    L_MAKE_LAMBDA:
    {
      struct RProc *p;
      const mrb_irep *nirep = irep->reps[b];

      if (c & OP_L_CAPTURE) {
        p = mrb_closure_new(mrb, nirep);
      }
      else {
        p = mrb_proc_new(mrb, nirep);
        p->flags |= MRB_PROC_SCOPE;
      }
      if (c & OP_L_STRICT) p->flags |= MRB_PROC_STRICT;
      regs[a] = mrb_obj_value(p);
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }
    CASE(OP_BLOCK, BB) {
      c = OP_L_BLOCK;
      goto L_MAKE_LAMBDA;
    }
    CASE(OP_METHOD, BB) {
      c = OP_L_METHOD;
      goto L_MAKE_LAMBDA;
    }

    CASE(OP_RANGE_INC, B) {
      mrb_value v = mrb_range_new(mrb, regs[a], regs[a+1], FALSE);
      regs[a] = v;
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_RANGE_EXC, B) {
      mrb_value v = mrb_range_new(mrb, regs[a], regs[a+1], TRUE);
      regs[a] = v;
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_OCLASS, B) {
      regs[a] = mrb_obj_value(mrb->object_class);
      NEXT;
    }

    CASE(OP_CLASS, BB) {
      struct RClass *c = 0, *baseclass;
      mrb_value base, super;
      mrb_sym id = syms[b];

      base = regs[a];
      super = regs[a+1];
      if (mrb_nil_p(base)) {
        baseclass = MRB_PROC_TARGET_CLASS(mrb->c->ci->proc);
        if (!baseclass) baseclass = mrb->object_class;
        base = mrb_obj_value(baseclass);
      }
      c = mrb_vm_define_class(mrb, base, super, id);
      regs[a] = mrb_obj_value(c);
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_MODULE, BB) {
      struct RClass *cls = 0, *baseclass;
      mrb_value base;
      mrb_sym id = syms[b];

      base = regs[a];
      if (mrb_nil_p(base)) {
        baseclass = MRB_PROC_TARGET_CLASS(mrb->c->ci->proc);
        if (!baseclass) baseclass = mrb->object_class;
        base = mrb_obj_value(baseclass);
      }
      cls = mrb_vm_define_module(mrb, base, id);
      regs[a] = mrb_obj_value(cls);
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_EXEC, BB)
    {
      mrb_value recv = regs[a];
      struct RProc *p;
      const mrb_irep *nirep = irep->reps[b];

      /* prepare closure */
      p = mrb_proc_new(mrb, nirep);
      p->c = NULL;
      mrb_field_write_barrier(mrb, (struct RBasic*)p, (struct RBasic*)proc);
      MRB_PROC_SET_TARGET_CLASS(p, mrb_class_ptr(recv));
      p->flags |= MRB_PROC_SCOPE;

      /* prepare call stack */
      cipush(mrb, a, 0, mrb_class_ptr(recv), p, 0, 0);

      irep = p->body.irep;
      pool = irep->pool;
      syms = irep->syms;
      mrb_stack_extend(mrb, irep->nregs);
      stack_clear(regs+1, irep->nregs-1);
      pc = irep->iseq;
      JUMP;
    }

    CASE(OP_DEF, BB) {
      struct RClass *target = mrb_class_ptr(regs[a]);
      struct RProc *p = mrb_proc_ptr(regs[a+1]);
      mrb_method_t m;
      mrb_sym mid = syms[b];

      MRB_METHOD_FROM_PROC(m, p);
      mrb_define_method_raw(mrb, target, mid, m);
      mrb_method_added(mrb, target, mid);
      mrb_gc_arena_restore(mrb, ai);
      regs[a] = mrb_symbol_value(mid);
      NEXT;
    }

    CASE(OP_SCLASS, B) {
      regs[a] = mrb_singleton_class(mrb, regs[a]);
      mrb_gc_arena_restore(mrb, ai);
      NEXT;
    }

    CASE(OP_TCLASS, B) {
      struct RClass *target = check_target_class(mrb);
      if (!target) goto L_RAISE;
      regs[a] = mrb_obj_value(target);
      NEXT;
    }

    CASE(OP_ALIAS, BB) {
      struct RClass *target = check_target_class(mrb);

      if (!target) goto L_RAISE;
      mrb_alias_method(mrb, target, syms[a], syms[b]);
      mrb_method_added(mrb, target, syms[a]);
      NEXT;
    }
    CASE(OP_UNDEF, B) {
      struct RClass *target = check_target_class(mrb);

      if (!target) goto L_RAISE;
      mrb_undef_method_id(mrb, target, syms[a]);
      NEXT;
    }

    CASE(OP_DEBUG, Z) {
      FETCH_BBB();
#ifdef MRB_USE_DEBUG_HOOK
      mrb->debug_op_hook(mrb, irep, pc, regs);
#else
#ifndef MRB_NO_STDIO
      printf("OP_DEBUG %d %d %d\n", a, b, c);
#else
      abort();
#endif
#endif
      NEXT;
    }

    CASE(OP_ERR, B) {
      size_t len = pool[a].tt >> 2;
      mrb_value exc;

      mrb_assert((pool[a].tt&IREP_TT_NFLAG)==0);
      exc = mrb_exc_new(mrb, E_LOCALJUMP_ERROR, pool[a].u.str, len);
      mrb_exc_set(mrb, exc);
      goto L_RAISE;
    }

    CASE(OP_EXT1, Z) {
      insn = READ_B();
      switch (insn) {
#define OPCODE(insn,ops) case OP_ ## insn: FETCH_ ## ops ## _1(); mrb->c->ci->pc = pc; goto L_OP_ ## insn ## _BODY;
#include "mruby/ops.h"
#undef OPCODE
      }
      pc--;
      NEXT;
    }
    CASE(OP_EXT2, Z) {
      insn = READ_B();
      switch (insn) {
#define OPCODE(insn,ops) case OP_ ## insn: FETCH_ ## ops ## _2(); mrb->c->ci->pc = pc; goto L_OP_ ## insn ## _BODY;
#include "mruby/ops.h"
#undef OPCODE
      }
      pc--;
      NEXT;
    }
    CASE(OP_EXT3, Z) {
      uint8_t insn = READ_B();
      switch (insn) {
#define OPCODE(insn,ops) case OP_ ## insn: FETCH_ ## ops ## _3(); mrb->c->ci->pc = pc; goto L_OP_ ## insn ## _BODY;
#include "mruby/ops.h"
#undef OPCODE
      }
      pc--;
      NEXT;
    }

    CASE(OP_STOP, Z) {
      /*        stop VM */
      CHECKPOINT_RESTORE(RBREAK_TAG_STOP) {
        /* do nothing */
      }
      CHECKPOINT_MAIN(RBREAK_TAG_STOP) {
        UNWIND_ENSURE(mrb, mrb->c->ci, pc, RBREAK_TAG_STOP, proc, mrb_nil_value());
      }
      CHECKPOINT_END(RBREAK_TAG_STOP);
    L_STOP:
      mrb->jmp = prev_jmp;
      if (mrb->exc) {
        mrb_assert(mrb->exc->tt == MRB_TT_EXCEPTION);
        return mrb_obj_value(mrb->exc);
      }
      return regs[irep->nlocals];
    }
  }
  END_DISPATCH;
#undef regs
  }
  MRB_CATCH(&c_jmp) {
    mrb_callinfo *ci = mrb->c->ci;
    while (ci > mrb->c->cibase && ci->cci == CINFO_DIRECT) {
      ci = cipop(mrb);
    }
    exc_catched = TRUE;
    pc = ci->pc;
    goto RETRY_TRY_BLOCK;
  }
  MRB_END_EXC(&c_jmp);
}