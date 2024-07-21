block_filter (void *opaque, int control, iobuf_t chain, byte * buffer,
	      size_t * ret_len)
{
  block_filter_ctx_t *a = opaque;
  char *buf = (char *)buffer;
  size_t size = *ret_len;
  int c, needed, rc = 0;
  char *p;

  if (control == IOBUFCTRL_UNDERFLOW)
    {
      size_t n = 0;

      p = buf;
      assert (size);		/* need a buffer */
      if (a->eof)		/* don't read any further */
	rc = -1;
      while (!rc && size)
	{
	  if (!a->size)
	    {			/* get the length bytes */
	      if (a->partial == 2)
		{
		  a->eof = 1;
		  if (!n)
		    rc = -1;
		  break;
		}
	      else if (a->partial)
		{
		  /* These OpenPGP introduced huffman like encoded length
		   * bytes are really a mess :-( */
		  if (a->first_c)
		    {
		      c = a->first_c;
		      a->first_c = 0;
		    }
		  else if ((c = iobuf_get (chain)) == -1)
		    {
		      log_error ("block_filter: 1st length byte missing\n");
		      rc = GPG_ERR_BAD_DATA;
		      break;
		    }
		  if (c < 192)
		    {
		      a->size = c;
		      a->partial = 2;
		      if (!a->size)
			{
			  a->eof = 1;
			  if (!n)
			    rc = -1;
			  break;
			}
		    }
		  else if (c < 224)
		    {
		      a->size = (c - 192) * 256;
		      if ((c = iobuf_get (chain)) == -1)
			{
			  log_error
			    ("block_filter: 2nd length byte missing\n");
			  rc = GPG_ERR_BAD_DATA;
			  break;
			}
		      a->size += c + 192;
		      a->partial = 2;
		      if (!a->size)
			{
			  a->eof = 1;
			  if (!n)
			    rc = -1;
			  break;
			}
		    }
		  else if (c == 255)
		    {
		      a->size = iobuf_get (chain) << 24;
		      a->size |= iobuf_get (chain) << 16;
		      a->size |= iobuf_get (chain) << 8;
		      if ((c = iobuf_get (chain)) == -1)
			{
			  log_error ("block_filter: invalid 4 byte length\n");
			  rc = GPG_ERR_BAD_DATA;
			  break;
			}
		      a->size |= c;
                      a->partial = 2;
                      if (!a->size)
                        {
                          a->eof = 1;
                          if (!n)
                            rc = -1;
                          break;
			}
		    }
		  else
		    { /* Next partial body length. */
		      a->size = 1 << (c & 0x1f);
		    }
		  /*  log_debug("partial: ctx=%p c=%02x size=%u\n", a, c, a->size); */
		}
	      else
		BUG ();
	    }

	  while (!rc && size && a->size)
	    {
	      needed = size < a->size ? size : a->size;
	      c = iobuf_read (chain, p, needed);
	      if (c < needed)
		{
		  if (c == -1)
		    c = 0;
		  log_error
		    ("block_filter %p: read error (size=%lu,a->size=%lu)\n",
		     a, (ulong) size + c, (ulong) a->size + c);
		  rc = GPG_ERR_BAD_DATA;
		}
	      else
		{
		  size -= c;
		  a->size -= c;
		  p += c;
		  n += c;
		}
	    }
	}
      *ret_len = n;
    }
  else if (control == IOBUFCTRL_FLUSH)
    {
      if (a->partial)
	{			/* the complicated openpgp scheme */
	  size_t blen, n, nbytes = size + a->buflen;

	  assert (a->buflen <= OP_MIN_PARTIAL_CHUNK);
	  if (nbytes < OP_MIN_PARTIAL_CHUNK)
	    {
	      /* not enough to write a partial block out; so we store it */
	      if (!a->buffer)
		a->buffer = xmalloc (OP_MIN_PARTIAL_CHUNK);
	      memcpy (a->buffer + a->buflen, buf, size);
	      a->buflen += size;
	    }
	  else
	    {			/* okay, we can write out something */
	      /* do this in a loop to use the most efficient block lengths */
	      p = buf;
	      do
		{
		  /* find the best matching block length - this is limited
		   * by the size of the internal buffering */
		  for (blen = OP_MIN_PARTIAL_CHUNK * 2,
		       c = OP_MIN_PARTIAL_CHUNK_2POW + 1; blen <= nbytes;
		       blen *= 2, c++)
		    ;
		  blen /= 2;
		  c--;
		  /* write the partial length header */
		  assert (c <= 0x1f);	/*;-) */
		  c |= 0xe0;
		  iobuf_put (chain, c);
		  if ((n = a->buflen))
		    {		/* write stuff from the buffer */
		      assert (n == OP_MIN_PARTIAL_CHUNK);
		      if (iobuf_write (chain, a->buffer, n))
			rc = gpg_error_from_syserror ();
		      a->buflen = 0;
		      nbytes -= n;
		    }
		  if ((n = nbytes) > blen)
		    n = blen;
		  if (n && iobuf_write (chain, p, n))
		    rc = gpg_error_from_syserror ();
		  p += n;
		  nbytes -= n;
		}
	      while (!rc && nbytes >= OP_MIN_PARTIAL_CHUNK);
	      /* store the rest in the buffer */
	      if (!rc && nbytes)
		{
		  assert (!a->buflen);
		  assert (nbytes < OP_MIN_PARTIAL_CHUNK);
		  if (!a->buffer)
		    a->buffer = xmalloc (OP_MIN_PARTIAL_CHUNK);
		  memcpy (a->buffer, p, nbytes);
		  a->buflen = nbytes;
		}
	    }
	}
      else
	BUG ();
    }
  else if (control == IOBUFCTRL_INIT)
    {
      if (DBG_IOBUF)
	log_debug ("init block_filter %p\n", a);
      if (a->partial)
	a->count = 0;
      else if (a->use == 1)
	a->count = a->size = 0;
      else
	a->count = a->size;	/* force first length bytes */
      a->eof = 0;
      a->buffer = NULL;
      a->buflen = 0;
    }
  else if (control == IOBUFCTRL_DESC)
    {
      *(char **) buf = "block_filter";
    }
  else if (control == IOBUFCTRL_FREE)
    {
      if (a->use == 2)
	{			/* write the end markers */
	  if (a->partial)
	    {
	      u32 len;
	      /* write out the remaining bytes without a partial header
	       * the length of this header may be 0 - but if it is
	       * the first block we are not allowed to use a partial header
	       * and frankly we can't do so, because this length must be
	       * a power of 2. This is _really_ complicated because we
	       * have to check the possible length of a packet prior
	       * to it's creation: a chain of filters becomes complicated
	       * and we need a lot of code to handle compressed packets etc.
	       *   :-(((((((
	       */
	      /* construct header */
	      len = a->buflen;
	      /*log_debug("partial: remaining length=%u\n", len ); */
	      if (len < 192)
		rc = iobuf_put (chain, len);
	      else if (len < 8384)
		{
		  if (!(rc = iobuf_put (chain, ((len - 192) / 256) + 192)))
		    rc = iobuf_put (chain, ((len - 192) % 256));
		}
	      else
		{		/* use a 4 byte header */
		  if (!(rc = iobuf_put (chain, 0xff)))
		    if (!(rc = iobuf_put (chain, (len >> 24) & 0xff)))
		      if (!(rc = iobuf_put (chain, (len >> 16) & 0xff)))
			if (!(rc = iobuf_put (chain, (len >> 8) & 0xff)))
			  rc = iobuf_put (chain, len & 0xff);
		}
	      if (!rc && len)
		rc = iobuf_write (chain, a->buffer, len);
	      if (rc)
		{
		  log_error ("block_filter: write error: %s\n",
			     strerror (errno));
		  rc = gpg_error_from_syserror ();
		}
	      xfree (a->buffer);
	      a->buffer = NULL;
	      a->buflen = 0;
	    }
	  else
	    BUG ();
	}
      else if (a->size)
	{
	  log_error ("block_filter: pending bytes!\n");
	}
      if (DBG_IOBUF)
	log_debug ("free block_filter %p\n", a);
      xfree (a);		/* we can free our context now */
    }

  return rc;
}