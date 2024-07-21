webSocketsHasDataInBuffer(rfbClientPtr cl)
{
    ws_ctx_t *wsctx = (ws_ctx_t *)cl->wsctx;

    if (wsctx && wsctx->readbuflen)
      return TRUE;

    return (cl->sslctx && rfbssl_pending(cl) > 0);
}