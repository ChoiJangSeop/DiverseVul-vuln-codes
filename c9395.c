static GCObject **correctgraylist (GCObject **p) {
  GCObject *curr;
  while ((curr = *p) != NULL) {
    switch (curr->tt) {
      case LUA_VTABLE: case LUA_VUSERDATA: {
        GCObject **next = getgclist(curr);
        if (getage(curr) == G_TOUCHED1) {  /* touched in this cycle? */
          lua_assert(isgray(curr));
          gray2black(curr);  /* make it black, for next barrier */
          changeage(curr, G_TOUCHED1, G_TOUCHED2);
          p = next;  /* go to next element */
        }
        else {  /* not touched in this cycle */
          if (!iswhite(curr)) {  /* not white? */
            lua_assert(isold(curr));
            if (getage(curr) == G_TOUCHED2)  /* advance from G_TOUCHED2... */
              changeage(curr, G_TOUCHED2, G_OLD);  /* ... to G_OLD */
            gray2black(curr);  /* make it black */
          }
          /* else, object is white: just remove it from this list */
          *p = *next;  /* remove 'curr' from gray list */
        }
        break;
      }
      case LUA_VTHREAD: {
        lua_State *th = gco2th(curr);
        lua_assert(!isblack(th));
        if (iswhite(th))  /* new object? */
          *p = th->gclist;  /* remove from gray list */
        else  /* old threads remain gray */
          p = &th->gclist;  /* go to next element */
        break;
      }
      default: lua_assert(0);  /* nothing more could be gray here */
    }
  }
  return p;
}