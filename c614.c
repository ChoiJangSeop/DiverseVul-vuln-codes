rfbSendAuthCaps(rfbClientPtr cl)
{
    rfbAuthenticationCapsMsg caps;
    rfbCapabilityInfo caplist[MAX_AUTH_CAPS];
    int count = 0;
    rfbTightClientPtr rtcp = rfbGetTightClientData(cl);

    rfbLog("tightvnc-filetransfer/rfbSendAuthCaps\n");

    if(rtcp == NULL)
	return;

    if (cl->screen->authPasswdData && !cl->reverseConnection) {
	/* chk if this condition is valid or not. */
	    SetCapInfo(&caplist[count], rfbAuthVNC, rfbStandardVendor);
	    rtcp->authCaps[count++] = rfbAuthVNC;
    }

    rtcp->nAuthCaps = count;
    caps.nAuthTypes = Swap32IfLE((uint32_t)count);
    if (rfbWriteExact(cl, (char *)&caps, sz_rfbAuthenticationCapsMsg) < 0) {
	rfbLogPerror("rfbSendAuthCaps: write");
	rfbCloseClient(cl);
	return;
    }

    if (count) {
	if (rfbWriteExact(cl, (char *)&caplist[0],
		       count * sz_rfbCapabilityInfo) < 0) {
	    rfbLogPerror("rfbSendAuthCaps: write");
	    rfbCloseClient(cl);
	    return;
	}
	/* Dispatch client input to rfbProcessClientAuthType. */
	/* Call the function for authentication from here */
	rfbProcessClientAuthType(cl);
    } else {
	/* Dispatch client input to rfbProcessClientInitMessage. */
	cl->state = RFB_INITIALISATION;
    }
}