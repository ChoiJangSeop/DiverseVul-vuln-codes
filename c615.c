void rfbFreeZrleData(rfbClientPtr cl)
{
  if (cl->zrleData)
    zrleOutStreamFree(cl->zrleData);
  cl->zrleData = NULL;
}