next_packet (unsigned char const **bufptr, size_t *buflen,
             unsigned char const **r_data, size_t *r_datalen, int *r_pkttype,
             size_t *r_ntotal)
{
  const unsigned char *buf = *bufptr;
  size_t len = *buflen;
  int c, ctb, pkttype;
  unsigned long pktlen;

  if (!len)
    return gpg_error (GPG_ERR_NO_DATA);

  ctb = *buf++; len--;
  if ( !(ctb & 0x80) )
    return gpg_error (GPG_ERR_INV_PACKET); /* Invalid CTB. */

  pktlen = 0;
  if ((ctb & 0x40))  /* New style (OpenPGP) CTB.  */
    {
      pkttype = (ctb & 0x3f);
      if (!len)
        return gpg_error (GPG_ERR_INV_PACKET); /* No 1st length byte. */
      c = *buf++; len--;
      if (pkttype == PKT_COMPRESSED)
        return gpg_error (GPG_ERR_UNEXPECTED); /* ... packet in a keyblock. */
      if ( c < 192 )
        pktlen = c;
      else if ( c < 224 )
        {
          pktlen = (c - 192) * 256;
          if (!len)
            return gpg_error (GPG_ERR_INV_PACKET); /* No 2nd length byte. */
          c = *buf++; len--;
          pktlen += c + 192;
        }
      else if (c == 255)
        {
          if (len <4 )
            return gpg_error (GPG_ERR_INV_PACKET); /* No length bytes. */
          pktlen  = (*buf++) << 24;
          pktlen |= (*buf++) << 16;
          pktlen |= (*buf++) << 8;
          pktlen |= (*buf++);
          len -= 4;
      }
      else /* Partial length encoding is not allowed for key packets. */
        return gpg_error (GPG_ERR_UNEXPECTED);
    }
  else /* Old style CTB.  */
    {
      int lenbytes;

      pktlen = 0;
      pkttype = (ctb>>2)&0xf;
      lenbytes = ((ctb&3)==3)? 0 : (1<<(ctb & 3));
      if (!lenbytes) /* Not allowed in key packets.  */
        return gpg_error (GPG_ERR_UNEXPECTED);
      if (len < lenbytes)
        return gpg_error (GPG_ERR_INV_PACKET); /* Not enough length bytes.  */
      for (; lenbytes; lenbytes--)
        {
          pktlen <<= 8;
          pktlen |= *buf++; len--;
	}
    }

  /* Do some basic sanity check.  */
  switch (pkttype)
    {
    case PKT_SIGNATURE:
    case PKT_SECRET_KEY:
    case PKT_PUBLIC_KEY:
    case PKT_SECRET_SUBKEY:
    case PKT_MARKER:
    case PKT_RING_TRUST:
    case PKT_USER_ID:
    case PKT_PUBLIC_SUBKEY:
    case PKT_OLD_COMMENT:
    case PKT_ATTRIBUTE:
    case PKT_COMMENT:
    case PKT_GPG_CONTROL:
      break; /* Okay these are allowed packets. */
    default:
      return gpg_error (GPG_ERR_UNEXPECTED);
    }

  if (pktlen == (unsigned long)(-1))
    return gpg_error (GPG_ERR_INV_PACKET);

  if (pktlen > len)
    return gpg_error (GPG_ERR_INV_PACKET); /* Packet length header too long. */

  *r_data = buf;
  *r_datalen = pktlen;
  *r_pkttype = pkttype;
  *r_ntotal = (buf - *bufptr) + pktlen;

  *bufptr = buf + pktlen;
  *buflen = len - pktlen;
  if (!*buflen)
    *bufptr = NULL;

  return 0;
}