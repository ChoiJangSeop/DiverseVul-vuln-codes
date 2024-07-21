byte* decode_base64(char* src,size_t ssize, size_t *ret_len)
{
  byte* outbuf;
  byte* retbuf;
  char* inb;
  int i;
  int l;
  int left;
  int pos;
  unsigned long triple;

  /* Exit on empty input */
  if (!ssize||src==NULL) {
    log_msg(LOG_LEVEL_DEBUG, "decode base64: empty string");
    return NULL;
  }


  /* Initialize working pointers */
  inb = src;
  outbuf = (byte *)checked_malloc(sizeof(byte)*B64_BUF);

  l = 0;
  triple = 0;
  pos=0;
  left = ssize;
  /*
   * Process entire inbuf.
   */
  while (left != 0)
    {
      left--;
      i = fromb64[(unsigned char)*inb];
      switch(i)
	{
	case FAIL:
	  log_msg(LOG_LEVEL_WARNING, "decode_base64: illegal character: '%c' in '%s'", *inb, src);
	  free(outbuf);
	  return NULL;
	  break;
	case SKIP:
	  break;
	default:
	  triple = triple<<6 | (0x3f & i);
	  l++;
	  break;
	}
      if (l == 4 || left == 0)
	{
	  switch(l)
	    {
	    case 2:
	      triple = triple>>4;
	      break;
	    case 3:
	      triple = triple>>2;
	      break;
	    default:
	      break;
	    }
	  for (l  -= 2; l >= 0; l--)
	    {
	      outbuf[pos]=( 0xff & (triple>>(l*8)));
	      pos++;
	    }
	  triple = 0;
	  l = 0;
	}
      inb++;
    }
  
  retbuf=(byte*)checked_malloc(sizeof(byte)*(pos+1));
  memcpy(retbuf,outbuf,pos);
  retbuf[pos]='\0';
  
  free(outbuf);

  if (ret_len) *ret_len = pos;
  
  return retbuf;
}