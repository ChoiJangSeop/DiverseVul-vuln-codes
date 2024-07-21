HandleUltraBPP (rfbClient* client, int rx, int ry, int rw, int rh)
{
  rfbZlibHeader hdr;
  int toRead=0;
  int inflateResult=0;
  lzo_uint uncompressedBytes = (( rw * rh ) * ( BPP / 8 ));

  if (!ReadFromRFBServer(client, (char *)&hdr, sz_rfbZlibHeader))
    return FALSE;

  toRead = rfbClientSwap32IfLE(hdr.nBytes);
  if (toRead==0) return TRUE;

  if (uncompressedBytes==0)
  {
      rfbClientLog("ultra error: rectangle has 0 uncomressed bytes ((%dw * %dh) * (%d / 8))\n", rw, rh, BPP); 
      return FALSE;
  }

  /* First make sure we have a large enough raw buffer to hold the
   * decompressed data.  In practice, with a fixed BPP, fixed frame
   * buffer size and the first update containing the entire frame
   * buffer, this buffer allocation should only happen once, on the
   * first update.
   */
  if ( client->raw_buffer_size < (int)uncompressedBytes) {
    if ( client->raw_buffer != NULL ) {
      free( client->raw_buffer );
    }
    client->raw_buffer_size = uncompressedBytes;
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
    /* buffer needs to be aligned on 4-byte boundaries */
    if ((client->ultra_buffer_size % 4)!=0)
      client->ultra_buffer_size += (4-(client->ultra_buffer_size % 4));
    client->ultra_buffer = (char*) malloc( client->ultra_buffer_size );
  }

  /* Fill the buffer, obtaining data from the server. */
  if (!ReadFromRFBServer(client, client->ultra_buffer, toRead))
      return FALSE;

  /* uncompress the data */
  uncompressedBytes = client->raw_buffer_size;
  inflateResult = lzo1x_decompress(
              (lzo_byte *)client->ultra_buffer, toRead,
              (lzo_byte *)client->raw_buffer, (lzo_uintp) &uncompressedBytes,
              NULL);
  
  
  if ((rw * rh * (BPP / 8)) != uncompressedBytes)
      rfbClientLog("Ultra decompressed too little (%d < %d)", (rw * rh * (BPP / 8)), uncompressedBytes);
  
  /* Put the uncompressed contents of the update on the screen. */
  if ( inflateResult == LZO_E_OK ) 
  {
    CopyRectangle(client, (unsigned char *)client->raw_buffer, rx, ry, rw, rh);
  }
  else
  {
    rfbClientLog("ultra decompress returned error: %d\n",
            inflateResult);
    return FALSE;
  }
  return TRUE;
}