rfbSendSetColourMapEntries(rfbClientPtr cl,
                           int firstColour,
                           int nColours)
{
    char buf[sz_rfbSetColourMapEntriesMsg + 256 * 3 * 2];
    char *wbuf = buf;
    rfbSetColourMapEntriesMsg *scme;
    uint16_t *rgb;
    rfbColourMap* cm = &cl->screen->colourMap;
    int i, len;

    if (nColours > 256) {
	/* some rare hardware has, e.g., 4096 colors cells: PseudoColor:12 */
    	wbuf = (char *) malloc(sz_rfbSetColourMapEntriesMsg + nColours * 3 * 2);
    }

    scme = (rfbSetColourMapEntriesMsg *)wbuf;
    rgb = (uint16_t *)(&wbuf[sz_rfbSetColourMapEntriesMsg]);

    scme->type = rfbSetColourMapEntries;

    scme->firstColour = Swap16IfLE(firstColour);
    scme->nColours = Swap16IfLE(nColours);

    len = sz_rfbSetColourMapEntriesMsg;

    for (i = 0; i < nColours; i++) {
      if(i<(int)cm->count) {
	if(cm->is16) {
	  rgb[i*3] = Swap16IfLE(cm->data.shorts[i*3]);
	  rgb[i*3+1] = Swap16IfLE(cm->data.shorts[i*3+1]);
	  rgb[i*3+2] = Swap16IfLE(cm->data.shorts[i*3+2]);
	} else {
	  rgb[i*3] = Swap16IfLE((unsigned short)cm->data.bytes[i*3]);
	  rgb[i*3+1] = Swap16IfLE((unsigned short)cm->data.bytes[i*3+1]);
	  rgb[i*3+2] = Swap16IfLE((unsigned short)cm->data.bytes[i*3+2]);
	}
      }
    }

    len += nColours * 3 * 2;

    if (rfbWriteExact(cl, wbuf, len) < 0) {
	rfbLogPerror("rfbSendSetColourMapEntries: write");
	rfbCloseClient(cl);
        if (wbuf != buf) free(wbuf);
	return FALSE;
    }

    rfbStatRecordMessageSent(cl, rfbSetColourMapEntries, len, len);
    if (wbuf != buf) free(wbuf);
    return TRUE;
}