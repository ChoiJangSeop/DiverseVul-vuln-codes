void rfbTightCleanup(rfbScreenInfoPtr screen)
{
  if(tightBeforeBufSize) {
    free(tightBeforeBuf);
    tightBeforeBufSize=0;
  }
  if(tightAfterBufSize) {
    free(tightAfterBuf);
    tightAfterBufSize=0;
  }
}