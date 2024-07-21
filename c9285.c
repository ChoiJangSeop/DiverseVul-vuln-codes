rfbSetClientColourMapBGR233(rfbClientPtr cl)
{
    char buf[sz_rfbSetColourMapEntriesMsg + 256 * 3 * 2];
    rfbSetColourMapEntriesMsg *scme = (rfbSetColourMapEntriesMsg *)buf;
    uint16_t *rgb = (uint16_t *)(&buf[sz_rfbSetColourMapEntriesMsg]);
    int i, len;
    int r, g, b;

    if (cl->format.bitsPerPixel != 8 ) {
        rfbErr("%s: client not 8 bits per pixel\n",
                "rfbSetClientColourMapBGR233");
        rfbCloseClient(cl);
        return FALSE;
    }
    
    scme->type = rfbSetColourMapEntries;

    scme->firstColour = Swap16IfLE(0);
    scme->nColours = Swap16IfLE(256);

    len = sz_rfbSetColourMapEntriesMsg;

    i = 0;

    for (b = 0; b < 4; b++) {
        for (g = 0; g < 8; g++) {
            for (r = 0; r < 8; r++) {
                rgb[i++] = Swap16IfLE(r * 65535 / 7);
                rgb[i++] = Swap16IfLE(g * 65535 / 7);
                rgb[i++] = Swap16IfLE(b * 65535 / 3);
            }
        }
    }

    len += 256 * 3 * 2;

    if (rfbWriteExact(cl, buf, len) < 0) {
        rfbLogPerror("rfbSetClientColourMapBGR233: write");
        rfbCloseClient(cl);
        return FALSE;
    }
    return TRUE;
}