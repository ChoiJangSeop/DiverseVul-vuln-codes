char* encode_base64(byte* src,size_t ssize)
{
  char* outbuf;
  char* retbuf;
  int pos;
  int i, l, left;
  unsigned long triple;
  byte *inb;
  
  /* Exit on empty input */
  if (!ssize||src==NULL){
    log_msg(LOG_LEVEL_DEBUG,"encode base64: empty string");
    return NULL;
  }
  outbuf = (char *)checked_malloc(sizeof(char)*B64_BUF);
  
  /* Initialize working pointers */
  inb = src;
  i = 0;
  triple = 0;
  pos = 0;
  left = ssize;
  log_msg(LOG_LEVEL_TRACE, "encode base64:, data length: %d", left);
  /*
   * Process entire inbuf.
   */
  while (left != 0)
    {
      i++;
      left--;
      
      triple = (triple <<8) | *inb;
      if (i == 3 || left == 0)
	{
	  switch (i) 
	    {
	    case 1:
	      triple = triple<<4;
	      break;
	    case 2:
	      triple = triple<<2;
	      break;
	    default:
	      break;
	    }
	  for (l = i; l >= 0; l--){
	    /* register */ 
	    int rr; 
	    rr = 0x3f & (triple>>(6*l)); 
	    assert (rr < 64); 
	    outbuf[pos]=tob64[rr];
	    pos++;
	      }
	  if (left == 0)
	    switch(i)
	      {
	      case 2:
		outbuf[pos]='=';
		pos++;
		break;
	      case 1:
		outbuf[pos]='=';
		pos++;
		outbuf[pos]='=';
		pos++;
		break;
	      default:
		break;
	      }
	  triple = 0;
	  i = 0;
	  }
      inb++;
  }
  
  /* outbuf is not completely used so we use retbuf */
  retbuf=(char*)checked_malloc(sizeof(char)*(pos+1));
  memcpy(retbuf,outbuf,pos);
  retbuf[pos]='\0';
  free(outbuf);

  return retbuf;
}