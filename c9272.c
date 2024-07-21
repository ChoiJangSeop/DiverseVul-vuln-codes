void luaT_adjustvarargs (lua_State *L, int nfixparams, CallInfo *ci,
                         const Proto *p) {
  int i;
  int actual = cast_int(L->top - ci->func) - 1;  /* number of arguments */
  int nextra = actual - nfixparams;  /* number of extra arguments */
  ci->u.l.nextraargs = nextra;
  checkstackGC(L, p->maxstacksize + 1);
  /* copy function to the top of the stack */
  setobjs2s(L, L->top++, ci->func);
  /* move fixed parameters to the top of the stack */
  for (i = 1; i <= nfixparams; i++) {
    setobjs2s(L, L->top++, ci->func + i);
    setnilvalue(s2v(ci->func + i));  /* erase original parameter (for GC) */
  }
  ci->func += actual + 1;
  ci->top += actual + 1;
  lua_assert(L->top <= ci->top && ci->top <= L->stack_last);
}