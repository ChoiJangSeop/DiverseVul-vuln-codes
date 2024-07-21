void luaD_call (lua_State *L, StkId func, int nresults) {
  lua_CFunction f;
 retry:
  switch (ttypetag(s2v(func))) {
    case LUA_VCCL:  /* C closure */
      f = clCvalue(s2v(func))->f;
      goto Cfunc;
    case LUA_VLCF:  /* light C function */
      f = fvalue(s2v(func));
     Cfunc: {
      int n;  /* number of returns */
      CallInfo *ci = next_ci(L);
      checkstackp(L, LUA_MINSTACK, func);  /* ensure minimum stack size */
      ci->nresults = nresults;
      ci->callstatus = CIST_C;
      ci->top = L->top + LUA_MINSTACK;
      ci->func = func;
      L->ci = ci;
      lua_assert(ci->top <= L->stack_last);
      if (L->hookmask & LUA_MASKCALL) {
        int narg = cast_int(L->top - func) - 1;
        luaD_hook(L, LUA_HOOKCALL, -1, 1, narg);
      }
      lua_unlock(L);
      n = (*f)(L);  /* do the actual call */
      lua_lock(L);
      api_checknelems(L, n);
      luaD_poscall(L, ci, n);
      break;
    }
    case LUA_VLCL: {  /* Lua function */
      CallInfo *ci = next_ci(L);
      Proto *p = clLvalue(s2v(func))->p;
      int narg = cast_int(L->top - func) - 1;  /* number of real arguments */
      int nfixparams = p->numparams;
      int fsize = p->maxstacksize;  /* frame size */
      checkstackp(L, fsize, func);
      ci->nresults = nresults;
      ci->u.l.savedpc = p->code;  /* starting point */
      ci->callstatus = 0;
      ci->top = func + 1 + fsize;
      ci->func = func;
      L->ci = ci;
      for (; narg < nfixparams; narg++)
        setnilvalue(s2v(L->top++));  /* complete missing arguments */
      lua_assert(ci->top <= L->stack_last);
      luaV_execute(L, ci);  /* run the function */
      break;
    }
    default: {  /* not a function */
      checkstackp(L, 1, func);  /* space for metamethod */
      luaD_tryfuncTM(L, func);  /* try to get '__call' metamethod */
      goto retry;  /* try again with metamethod */
    }
  }
}