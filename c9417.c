static TValue *trace_exit_cp(lua_State *L, lua_CFunction dummy, void *ud)
{
  ExitDataCP *exd = (ExitDataCP *)ud;
  cframe_errfunc(L->cframe) = -1;  /* Inherit error function. */
  exd->pc = lj_snap_restore(exd->J, exd->exptr);
  UNUSED(dummy);
  return NULL;
}