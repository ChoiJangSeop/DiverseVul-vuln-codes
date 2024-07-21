static BCPos debug_framepc(lua_State *L, GCfunc *fn, cTValue *nextframe)
{
  const BCIns *ins;
  GCproto *pt;
  BCPos pos;
  lua_assert(fn->c.gct == ~LJ_TFUNC || fn->c.gct == ~LJ_TTHREAD);
  if (!isluafunc(fn)) {  /* Cannot derive a PC for non-Lua functions. */
    return NO_BCPOS;
  } else if (nextframe == NULL) {  /* Lua function on top. */
    void *cf = cframe_raw(L->cframe);
    if (cf == NULL || (char *)cframe_pc(cf) == (char *)cframe_L(cf))
      return NO_BCPOS;
    ins = cframe_pc(cf);  /* Only happens during error/hook handling. */
  } else {
    if (frame_islua(nextframe)) {
      ins = frame_pc(nextframe);
    } else if (frame_iscont(nextframe)) {
      ins = frame_contpc(nextframe);
    } else {
      /* Lua function below errfunc/gc/hook: find cframe to get the PC. */
      void *cf = cframe_raw(L->cframe);
      TValue *f = L->base-1;
      for (;;) {
	if (cf == NULL)
	  return NO_BCPOS;
	while (cframe_nres(cf) < 0) {
	  if (f >= restorestack(L, -cframe_nres(cf)))
	    break;
	  cf = cframe_raw(cframe_prev(cf));
	  if (cf == NULL)
	    return NO_BCPOS;
	}
	if (f < nextframe)
	  break;
	if (frame_islua(f)) {
	  f = frame_prevl(f);
	} else {
	  if (frame_isc(f) || (LJ_HASFFI && frame_iscont(f) &&
			       (f-1)->u32.lo == LJ_CONT_FFI_CALLBACK))
	    cf = cframe_raw(cframe_prev(cf));
	  f = frame_prevd(f);
	}
      }
      ins = cframe_pc(cf);
    }
  }
  pt = funcproto(fn);
  pos = proto_bcpos(pt, ins) - 1;
#if LJ_HASJIT
  if (pos > pt->sizebc) {  /* Undo the effects of lj_trace_exit for JLOOP. */
    GCtrace *T = (GCtrace *)((char *)(ins-1) - offsetof(GCtrace, startins));
    lua_assert(bc_isret(bc_op(ins[-1])));
    pos = proto_bcpos(pt, mref(T->startpc, const BCIns));
  }
#endif
  return pos;
}