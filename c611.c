clientOutput(void *data)
{
    rfbClientPtr cl = (rfbClientPtr)data;
    rfbBool haveUpdate;
    sraRegion* updateRegion;

    while (1) {
        haveUpdate = false;
        while (!haveUpdate) {
            if (cl->sock == -1) {
                /* Client has disconnected. */
                return NULL;
            }
	    LOCK(cl->updateMutex);
	    haveUpdate = FB_UPDATE_PENDING(cl);
	    if(!haveUpdate) {
		updateRegion = sraRgnCreateRgn(cl->modifiedRegion);
		haveUpdate = sraRgnAnd(updateRegion,cl->requestedRegion);
		sraRgnDestroy(updateRegion);
	    }

            if (!haveUpdate) {
                WAIT(cl->updateCond, cl->updateMutex);
            }
	    UNLOCK(cl->updateMutex);
        }
        
        /* OK, now, to save bandwidth, wait a little while for more
           updates to come along. */
        usleep(cl->screen->deferUpdateTime * 1000);

        /* Now, get the region we're going to update, and remove
           it from cl->modifiedRegion _before_ we send the update.
           That way, if anything that overlaps the region we're sending
           is updated, we'll be sure to do another update later. */
        LOCK(cl->updateMutex);
	updateRegion = sraRgnCreateRgn(cl->modifiedRegion);
        UNLOCK(cl->updateMutex);

        /* Now actually send the update. */
	rfbIncrClientRef(cl);
        rfbSendFramebufferUpdate(cl, updateRegion);
	rfbDecrClientRef(cl);

	sraRgnDestroy(updateRegion);
    }

    /* Not reached. */
    return NULL;
}