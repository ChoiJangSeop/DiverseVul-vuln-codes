void lzxd_free(struct lzxd_stream *lzx) {
  struct mspack_system *sys;
  if (lzx) {
    sys = lzx->sys;
    sys->free(lzx->inbuf);
    sys->free(lzx->window);
    sys->free(lzx);
  }
}