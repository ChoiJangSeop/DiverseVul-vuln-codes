ProcShmCreatePixmap(client)
    register ClientPtr client;
{
    PixmapPtr pMap;
    DrawablePtr pDraw;
    DepthPtr pDepth;
    register int i, rc;
    ShmDescPtr shmdesc;
    REQUEST(xShmCreatePixmapReq);
    unsigned int width, height, depth;
    unsigned long size;

    REQUEST_SIZE_MATCH(xShmCreatePixmapReq);
    client->errorValue = stuff->pid;
    if (!sharedPixmaps)
	return BadImplementation;
    LEGAL_NEW_RESOURCE(stuff->pid, client);
    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, M_ANY,
			   DixGetAttrAccess);
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
    pMap = (*shmFuncs[pDraw->pScreen->myNum]->CreatePixmap)(
			    pDraw->pScreen, stuff->width,
			    stuff->height, stuff->depth,
			    shmdesc->addr + stuff->offset);
    if (pMap)
    {
	rc = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pid, RT_PIXMAP,
		      pMap, RT_NONE, NULL, DixCreateAccess);
	if (rc != Success) {
	    pDraw->pScreen->DestroyPixmap(pMap);
	    return rc;
	}
	dixSetPrivate(&pMap->devPrivates, shmPixmapPrivate, shmdesc);
	shmdesc->refcnt++;
	pMap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	pMap->drawable.id = stuff->pid;
	if (AddResource(stuff->pid, RT_PIXMAP, (pointer)pMap))
	{
	    return(client->noClientException);
	}
	pDraw->pScreen->DestroyPixmap(pMap);
    }
    return (BadAlloc);
}