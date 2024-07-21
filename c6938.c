rfbBool ConnectToRFBRepeater(rfbClient* client,const char *repeaterHost, int repeaterPort, const char *destHost, int destPort)
{
  rfbProtocolVersionMsg pv;
  int major,minor;
  char tmphost[250];

#ifdef LIBVNCSERVER_IPv6
  client->sock = ConnectClientToTcpAddr6(repeaterHost, repeaterPort);
  if (client->sock == -1)
#endif
  {
    unsigned int host;
    if (!StringToIPAddr(repeaterHost, &host)) {
      rfbClientLog("Couldn't convert '%s' to host address\n", repeaterHost);
      return FALSE;
    }

    client->sock = ConnectClientToTcpAddr(host, repeaterPort);
  }

  if (client->sock < 0) {
    rfbClientLog("Unable to connect to VNC repeater\n");
    return FALSE;
  }

  if (!SetNonBlocking(client->sock))
    return FALSE;

  if (!ReadFromRFBServer(client, pv, sz_rfbProtocolVersionMsg))
    return FALSE;
  pv[sz_rfbProtocolVersionMsg] = 0;

  /* UltraVNC repeater always report version 000.000 to identify itself */
  if (sscanf(pv,rfbProtocolVersionFormat,&major,&minor) != 2 || major != 0 || minor != 0) {
    rfbClientLog("Not a valid VNC repeater (%s)\n",pv);
    return FALSE;
  }

  rfbClientLog("Connected to VNC repeater, using protocol version %d.%d\n", major, minor);

  snprintf(tmphost, sizeof(tmphost), "%s:%d", destHost, destPort);
  if (!WriteToRFBServer(client, tmphost, sizeof(tmphost)))
    return FALSE;

  return TRUE;
}