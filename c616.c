rfbBool rfbSendFileTransferMessage(rfbClientPtr cl, uint8_t contentType, uint8_t contentParam, uint32_t size, uint32_t length, char *buffer)
{
    rfbFileTransferMsg ft;
    ft.type = rfbFileTransfer;
    ft.contentType = contentType;
    ft.contentParam = contentParam;
    ft.pad          = 0; /* UltraVNC did not Swap16LE(ft.contentParam) (Looks like it might be BigEndian) */
    ft.size         = Swap32IfLE(size);
    ft.length       = Swap32IfLE(length);
    
    FILEXFER_ALLOWED_OR_CLOSE_AND_RETURN("", cl, FALSE);
    /*
    rfbLog("rfbSendFileTransferMessage( %dtype, %dparam, %dsize, %dlen, %p)\n", contentType, contentParam, size, length, buffer);
    */
    if (rfbWriteExact(cl, (char *)&ft, sz_rfbFileTransferMsg) < 0) {
        rfbLogPerror("rfbSendFileTransferMessage: write");
        rfbCloseClient(cl);
        return FALSE;
    }

    if (length>0)
    {
        if (rfbWriteExact(cl, buffer, length) < 0) {
            rfbLogPerror("rfbSendFileTransferMessage: write");
            rfbCloseClient(cl);
            return FALSE;
        }
    }

    rfbStatRecordMessageSent(cl, rfbFileTransfer, sz_rfbFileTransferMsg+length, sz_rfbFileTransferMsg+length);

    return TRUE;
}