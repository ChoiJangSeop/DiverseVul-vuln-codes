static void atomic2gen (lua_State *L, global_State *g) {
  /* sweep all elements making them old */
  sweep2old(L, &g->allgc);
  /* everything alive now is old */
  g->reallyold = g->old = g->survival = g->allgc;

  /* repeat for 'finobj' lists */
  sweep2old(L, &g->finobj);
  g->finobjrold = g->finobjold = g->finobjsur = g->finobj;

  sweep2old(L, &g->tobefnz);

  g->gckind = KGC_GEN;
  g->lastatomic = 0;
  g->GCestimate = gettotalbytes(g);  /* base for memory control */
  finishgencycle(L, g);
}