static int remarkupvals (global_State *g) {
  lua_State *thread;
  lua_State **p = &g->twups;
  int work = 0;
  while ((thread = *p) != NULL) {
    work++;
    lua_assert(!isblack(thread));  /* threads are never black */
    if (isgray(thread) && thread->openupval != NULL)
      p = &thread->twups;  /* keep marked thread with upvalues in the list */
    else {  /* thread is not marked or without upvalues */
      UpVal *uv;
      *p = thread->twups;  /* remove thread from the list */
      thread->twups = thread;  /* mark that it is out of list */
      for (uv = thread->openupval; uv != NULL; uv = uv->u.open.next) {
        work++;
        if (!iswhite(uv))  /* upvalue already visited? */
          markvalue(g, uv->v);  /* mark its value */
      }
    }
  }
  return work;
}