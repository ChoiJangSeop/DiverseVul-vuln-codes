SendKeyEvent(rfbClient* client, uint32_t key, rfbBool down)
{
  rfbKeyEventMsg ke;

  if (!SupportsClient2Server(client, rfbKeyEvent)) return TRUE;

  ke.type = rfbKeyEvent;
  ke.down = down ? 1 : 0;
  ke.key = rfbClientSwap32IfLE(key);
  return WriteToRFBServer(client, (char *)&ke, sz_rfbKeyEventMsg);
}