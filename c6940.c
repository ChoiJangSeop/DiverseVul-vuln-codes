HandleUltraZipBPP (rfbClient* client, int rx, int ry, int rw, int rh)
{
  rfbZlibHeader hdr;
  int i=0;
  int toRead=0;
  int inflateResult=0;
  unsigned char *ptr=NULL;
  lzo_uint uncompressedBytes = ry + (rw * 65535);
  unsigned int numCacheRects = rx;

  if (!ReadFromRFBServer(client, (char *)&hdr, sz_rfbZlibHeader))
    return FALSE;

  toRead = rfbClientSwap32IfLE(hdr.nBytes);

  if (toRead==0) return TRUE;

  if (uncompressedBytes==0)
  {
      rfbClientLog("ultrazip error: rectangle has 0 uncomressed bytes (%dy + (%dw * 65535)) (%d rectangles)\n", ry, rw, rx); 
      return FALSE;
  }

  /* First make sure we have a large enough raw buffer to hold the
   * decompressed data.  In practice, with a fixed BPP, fixed frame
   * buffer size and the first update containing the entire frame
   * buffer, this buffer allocation should only happen once, on the
   * first update.
   */
  if ( client->raw_buffer_size < (int)(uncompressedBytes + 500)) {
    if ( client->raw_buffer != NULL ) {
      free( client->raw_buffer );
    }
    client->raw_buffer_size = uncompressedBytes + 500;
    /* buffer needs to be aligned on 4-byte boundaries */
    if ((client->raw_buffer_size % 4)!=0)
      client->raw_buffer_size += (4-(client->raw_buffer_size % 4));
    client->raw_buffer = (char*) malloc( client->raw_buffer_size );
  }

 
  /* allocate enough space to store the incoming compressed packet */
  if ( client->ultra_buffer_size < toRead ) {
    if ( client->ultra_buffer != NULL ) {
      free( client->ultra_buffer );
    }
    client->ultra_buffer_size = toRead;
    client->ultra_buffer = (char*) malloc( client->ultra_buffer_size );
  }

  /* Fill the buffer, obtaining data from the server. */
  if (!ReadFromRFBServer(client, client->ultra_buffer, toRead))
      return FALSE;

  /* uncompress the data */
  uncompressedBytes = client->raw_buffer_size;
  inflateResult = lzo1x_decompress_safe(
              (lzo_byte *)client->ultra_buffer, toRead,
              (lzo_byte *)client->raw_buffer, &uncompressedBytes, NULL);
  if ( inflateResult != LZO_E_OK ) 
  {
    rfbClientLog("ultra decompress returned error: %d\n",
            inflateResult);
    return FALSE;
  }
  
  /* Put the uncompressed contents of the update on the screen. */
  ptr = (unsigned char *)client->raw_buffer;
  for (i=0; i<numCacheRects; i++)
  {
    unsigned short sx, sy, sw, sh;
    unsigned int se;

    memcpy((char *)&sx, ptr, 2); ptr += 2;
    memcpy((char *)&sy, ptr, 2); ptr += 2;
    memcpy((char *)&sw, ptr, 2); ptr += 2;
    memcpy((char *)&sh, ptr, 2); ptr += 2;
    memcpy((char *)&se, ptr, 4); ptr += 4;

    sx = rfbClientSwap16IfLE(sx);
    sy = rfbClientSwap16IfLE(sy);
    sw = rfbClientSwap16IfLE(sw);
    sh = rfbClientSwap16IfLE(sh);
    se = rfbClientSwap32IfLE(se);

    if (se == rfbEncodingRaw)
    {
        client->GotBitmap(client, (unsigned char *)ptr, sx, sy, sw, sh);
        ptr += ((sw * sh) * (BPP / 8));
    }
  }  

  return TRUE;
}