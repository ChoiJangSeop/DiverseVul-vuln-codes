void luaT_getvarargs (lua_State *L, CallInfo *ci, StkId where, int wanted) {
  int i;
  int nextra = ci->u.l.nextraargs;
  if (wanted < 0) {
    wanted = nextra;  /* get all extra arguments available */
    checkstackp(L, nextra, where);  /* ensure stack space */
    L->top = where + nextra;  /* next instruction will need top */
  }
  for (i = 0; i < wanted && i < nextra; i++)
    setobjs2s(L, where + i, ci->func - nextra + i);
  for (; i < wanted; i++)   /* complete required results with nil */
    setnilvalue(s2v(where + i));
}