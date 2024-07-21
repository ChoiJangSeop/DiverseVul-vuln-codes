ProcPanoramiXShmCreatePixmap(
    register ClientPtr client)
{
    ScreenPtr pScreen = NULL;
    PixmapPtr pMap = NULL;
    DrawablePtr pDraw;
    DepthPtr pDepth;
    int i, j, result, rc;
    ShmDescPtr shmdesc;
    REQUEST(xShmCreatePixmapReq);
    unsigned int width, height, depth;
    unsigned long size;
    PanoramiXRes *newPix;

    REQUEST_SIZE_MATCH(xShmCreatePixmapReq);
    client->errorValue = stuff->pid;
    if (!sharedPixmaps)
	return BadImplementation;
    LEGAL_NEW_RESOURCE(stuff->pid, client);
    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, M_ANY,
			   DixUnknownAccess);
    if (rc != Success)
	return rc;

    VERIFY_SHMPTR(stuff->shmseg, stuff->offset, TRUE, shmdesc, client);

    width = stuff->width;
    height = stuff->height;
    depth = stuff->depth;
    if (!width || !height || !depth)
    {
	client->errorValue = 0;
        return BadValue;
    }
    if (width > 32767 || height > 32767)
        return BadAlloc;

    if (stuff->depth != 1)
    {
        pDepth = pDraw->pScreen->allowedDepths;
        for (i=0; i<pDraw->pScreen->numDepths; i++, pDepth++)
	   if (pDepth->depth == stuff->depth)
               goto CreatePmap;
	client->errorValue = stuff->depth;
        return BadValue;
    }

CreatePmap:
    size = PixmapBytePad(width, depth) * height;
    if (sizeof(size) == 4 && BitsPerPixel(depth) > 8) {
        if (size < width * height)
            return BadAlloc;
        /* thankfully, offset is unsigned */
        if (stuff->offset + size < size)
            return BadAlloc;
    }

    VERIFY_SHMSIZE(shmdesc, stuff->offset, size, client);

    if(!(newPix = (PanoramiXRes *) xalloc(sizeof(PanoramiXRes))))
	return BadAlloc;

    newPix->type = XRT_PIXMAP;
    newPix->u.pix.shared = TRUE;
    newPix->info[0].id = stuff->pid;
    for(j = 1; j < PanoramiXNumScreens; j++)
	newPix->info[j].id = FakeClientID(client->index);

    result = (client->noClientException);

    FOR_NSCREENS(j) {
	pScreen = screenInfo.screens[j];

	pMap = (*shmFuncs[j]->CreatePixmap)(pScreen, 
				stuff->width, stuff->height, stuff->depth,
				shmdesc->addr + stuff->offset);

	if (pMap) {
	    dixSetPrivate(&pMap->devPrivates, shmPixmapPrivate, shmdesc);
            shmdesc->refcnt++;
	    pMap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	    pMap->drawable.id = newPix->info[j].id;
	    if (!AddResource(newPix->info[j].id, RT_PIXMAP, (pointer)pMap)) {
		(*pScreen->DestroyPixmap)(pMap);
		result = BadAlloc;
		break;
	    }
	} else {
	   result = BadAlloc;
	   break;
	}
    }

    if(result == BadAlloc) {
	while(j--) {
	    (*pScreen->DestroyPixmap)(pMap);
	    FreeResource(newPix->info[j].id, RT_NONE);
	}
	xfree(newPix);
    } else 
	AddResource(stuff->pid, XRT_PIXMAP, newPix);

    return result;
}