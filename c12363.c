SwapCreateRegister(ClientPtr client, xRecordRegisterClientsReq * stuff)
{
    int i;
    XID *pClientID;

    swapl(&stuff->context);
    swapl(&stuff->nClients);
    swapl(&stuff->nRanges);
    pClientID = (XID *) &stuff[1];
    if (stuff->nClients >
        client->req_len - bytes_to_int32(sz_xRecordRegisterClientsReq))
        return BadLength;
    for (i = 0; i < stuff->nClients; i++, pClientID++) {
        swapl(pClientID);
    }
    if (stuff->nRanges >
        client->req_len - bytes_to_int32(sz_xRecordRegisterClientsReq)
        - stuff->nClients)
        return BadLength;
    RecordSwapRanges((xRecordRange *) pClientID, stuff->nRanges);
    return Success;
}                               /* SwapCreateRegister */