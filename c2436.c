enum_sig_subpkt (const subpktarea_t * pktbuf, sigsubpkttype_t reqtype,
		 size_t * ret_n, int *start, int *critical)
{
  const byte *buffer;
  int buflen;
  int type;
  int critical_dummy;
  int offset;
  size_t n;
  int seq = 0;
  int reqseq = start ? *start : 0;

  if (!critical)
    critical = &critical_dummy;

  if (!pktbuf || reqseq == -1)
    {
      static char dummy[] = "x";
      /* Return a value different from NULL to indicate that
       * there is no critical bit we do not understand.  */
      return reqtype ==	SIGSUBPKT_TEST_CRITICAL ? dummy : NULL;
    }
  buffer = pktbuf->data;
  buflen = pktbuf->len;
  while (buflen)
    {
      n = *buffer++;
      buflen--;
      if (n == 255) /* 4 byte length header.  */
	{
	  if (buflen < 4)
	    goto too_short;
	  n = (buffer[0] << 24) | (buffer[1] << 16)
	    | (buffer[2] << 8) | buffer[3];
	  buffer += 4;
	  buflen -= 4;
	}
      else if (n >= 192) /* 4 byte special encoded length header.  */
	{
	  if (buflen < 2)
	    goto too_short;
	  n = ((n - 192) << 8) + *buffer + 192;
	  buffer++;
	  buflen--;
	}
      if (buflen < n)
	goto too_short;
      type = *buffer;
      if (type & 0x80)
	{
	  type &= 0x7f;
	  *critical = 1;
	}
      else
	*critical = 0;
      if (!(++seq > reqseq))
	;
      else if (reqtype == SIGSUBPKT_TEST_CRITICAL)
	{
	  if (*critical)
	    {
	      if (n - 1 > buflen + 1)
		goto too_short;
	      if (!can_handle_critical (buffer + 1, n - 1, type))
		{
		  if (opt.verbose)
		    log_info (_("subpacket of type %d has "
				"critical bit set\n"), type);
		  if (start)
		    *start = seq;
		  return NULL;	/* This is an error.  */
		}
	    }
	}
      else if (reqtype < 0) /* List packets.  */
	dump_sig_subpkt (reqtype == SIGSUBPKT_LIST_HASHED,
			 type, *critical, buffer, buflen, n);
      else if (type == reqtype) /* Found.  */
	{
	  buffer++;
	  n--;
	  if (n > buflen)
	    goto too_short;
	  if (ret_n)
	    *ret_n = n;
	  offset = parse_one_sig_subpkt (buffer, n, type);
	  switch (offset)
	    {
	    case -2:
	      log_error ("subpacket of type %d too short\n", type);
	      return NULL;
	    case -1:
	      return NULL;
	    default:
	      break;
	    }
	  if (start)
	    *start = seq;
	  return buffer + offset;
	}
      buffer += n;
      buflen -= n;
    }
  if (reqtype == SIGSUBPKT_TEST_CRITICAL)
    return buffer;  /* Used as True to indicate that there is no. */

  /* Critical bit we don't understand. */
  if (start)
    *start = -1;
  return NULL;	/* End of packets; not found.  */

 too_short:
  if (opt.verbose)
    log_info ("buffer shorter than subpacket\n");
  if (start)
    *start = -1;
  return NULL;
}