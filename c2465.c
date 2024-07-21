parse_attribute_subpkts (PKT_user_id * uid)
{
  size_t n;
  int count = 0;
  struct user_attribute *attribs = NULL;
  const byte *buffer = uid->attrib_data;
  int buflen = uid->attrib_len;
  byte type;

  xfree (uid->attribs);

  while (buflen)
    {
      n = *buffer++;
      buflen--;
      if (n == 255)  /* 4 byte length header.  */
	{
	  if (buflen < 4)
	    goto too_short;
	  n = (buffer[0] << 24) | (buffer[1] << 16)
	    | (buffer[2] << 8) | buffer[3];
	  buffer += 4;
	  buflen -= 4;
	}
      else if (n >= 192)  /* 2 byte special encoded length header.  */
	{
	  if (buflen < 2)
	    goto too_short;
	  n = ((n - 192) << 8) + *buffer + 192;
	  buffer++;
	  buflen--;
	}
      if (buflen < n)
	goto too_short;

      if (!n)
        {
          /* Too short to encode the subpacket type.  */
          if (opt.verbose)
            log_info ("attribute subpacket too short\n");
          break;
        }

      attribs = xrealloc (attribs,
                          (count + 1) * sizeof (struct user_attribute));
      memset (&attribs[count], 0, sizeof (struct user_attribute));

      type = *buffer;
      buffer++;
      buflen--;
      n--;

      attribs[count].type = type;
      attribs[count].data = buffer;
      attribs[count].len = n;
      buffer += n;
      buflen -= n;
      count++;
    }

  uid->attribs = attribs;
  uid->numattribs = count;
  return count;

 too_short:
  if (opt.verbose)
    log_info ("buffer shorter than attribute subpacket\n");
  uid->attribs = attribs;
  uid->numattribs = count;
  return count;
}