rfbSendServerCutText(rfbScreenInfoPtr rfbScreen,char *str, int len)
{
    rfbClientPtr cl;
    rfbServerCutTextMsg sct;
    rfbClientIteratorPtr iterator;

    iterator = rfbGetClientIterator(rfbScreen);
    while ((cl = rfbClientIteratorNext(iterator)) != NULL) {
        sct.type = rfbServerCutText;
        sct.length = Swap32IfLE(len);
        LOCK(cl->sendMutex);
        if (rfbWriteExact(cl, (char *)&sct,
                       sz_rfbServerCutTextMsg) < 0) {
            rfbLogPerror("rfbSendServerCutText: write");
            rfbCloseClient(cl);
            UNLOCK(cl->sendMutex);
            continue;
        }
        if (rfbWriteExact(cl, str, len) < 0) {
            rfbLogPerror("rfbSendServerCutText: write");
            rfbCloseClient(cl);
        }
        UNLOCK(cl->sendMutex);
        rfbStatRecordMessageSent(cl, rfbServerCutText, sz_rfbServerCutTextMsg+len, sz_rfbServerCutTextMsg+len);
    }
    rfbReleaseClientIterator(iterator);
}