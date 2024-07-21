ReadReason(rfbClient* client)
{
    uint32_t reasonLen;
    char *reason;

    /* we have an error following */
    if (!ReadFromRFBServer(client, (char *)&reasonLen, 4)) return;
    reasonLen = rfbClientSwap32IfLE(reasonLen);
    reason = malloc((uint64_t)reasonLen+1);
    if (!ReadFromRFBServer(client, reason, reasonLen)) { free(reason); return; }
    reason[reasonLen]=0;
    rfbClientLog("VNC connection failed: %s\n",reason);
    free(reason);
}