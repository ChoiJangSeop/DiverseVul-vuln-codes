rfbProcessClientAuthType(rfbClientPtr cl)
{
    uint32_t auth_type;
    int n, i;
    rfbTightClientPtr rtcp = rfbGetTightClientData(cl);

    rfbLog("tightvnc-filetransfer/rfbProcessClientAuthType\n");

    if(rtcp == NULL)
	return;

    /* Read authentication type selected by the client. */
    n = rfbReadExact(cl, (char *)&auth_type, sizeof(auth_type));
    if (n <= 0) {
	if (n == 0)
	    rfbLog("rfbProcessClientAuthType: client gone\n");
	else
	    rfbLogPerror("rfbProcessClientAuthType: read");
	rfbCloseClient(cl);
	return;
    }
    auth_type = Swap32IfLE(auth_type);

    /* Make sure it was present in the list sent by the server. */
    for (i = 0; i < rtcp->nAuthCaps; i++) {
	if (auth_type == rtcp->authCaps[i])
	    break;
    }
    if (i >= rtcp->nAuthCaps) {
	rfbLog("rfbProcessClientAuthType: "
	       "wrong authentication type requested\n");
	rfbCloseClient(cl);
	return;
    }

    switch (auth_type) {
    case rfbAuthNone:
	/* Dispatch client input to rfbProcessClientInitMessage. */
	cl->state = RFB_INITIALISATION;
	break;
    case rfbAuthVNC:
	rfbVncAuthSendChallenge(cl);
	break;
    default:
	rfbLog("rfbProcessClientAuthType: unknown authentication scheme\n");
	rfbCloseClient(cl);
    }
}