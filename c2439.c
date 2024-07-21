parse (IOBUF inp, PACKET * pkt, int onlykeypkts, off_t * retpos,
       int *skip, IOBUF out, int do_skip
#ifdef DEBUG_PARSE_PACKET
       , const char *dbg_w, const char *dbg_f, int dbg_l
#endif
       )
{
  int rc = 0, c, ctb, pkttype, lenbytes;
  unsigned long pktlen;
  byte hdr[8];
  int hdrlen;
  int new_ctb = 0, partial = 0;
  int with_uid = (onlykeypkts == 2);
  off_t pos;

  *skip = 0;
  assert (!pkt->pkt.generic);
  if (retpos || list_mode)
    {
      pos = iobuf_tell (inp);
      if (retpos)
        *retpos = pos;
    }
  else
    pos = 0; /* (silence compiler warning) */

  if ((ctb = iobuf_get (inp)) == -1)
    {
      rc = -1;
      goto leave;
    }
  hdrlen = 0;
  hdr[hdrlen++] = ctb;
  if (!(ctb & 0x80))
    {
      log_error ("%s: invalid packet (ctb=%02x)\n", iobuf_where (inp), ctb);
      rc = gpg_error (GPG_ERR_INV_PACKET);
      goto leave;
    }
  pktlen = 0;
  new_ctb = !!(ctb & 0x40);
  if (new_ctb)
    {
      pkttype = ctb & 0x3f;
      if ((c = iobuf_get (inp)) == -1)
	{
	  log_error ("%s: 1st length byte missing\n", iobuf_where (inp));
	  rc = gpg_error (GPG_ERR_INV_PACKET);
	  goto leave;
	}


      hdr[hdrlen++] = c;
      if (c < 192)
        pktlen = c;
      else if (c < 224)
        {
          pktlen = (c - 192) * 256;
          if ((c = iobuf_get (inp)) == -1)
            {
              log_error ("%s: 2nd length byte missing\n",
                         iobuf_where (inp));
              rc = gpg_error (GPG_ERR_INV_PACKET);
              goto leave;
            }
          hdr[hdrlen++] = c;
          pktlen += c + 192;
        }
      else if (c == 255)
        {
          pktlen = (hdr[hdrlen++] = iobuf_get_noeof (inp)) << 24;
          pktlen |= (hdr[hdrlen++] = iobuf_get_noeof (inp)) << 16;
          pktlen |= (hdr[hdrlen++] = iobuf_get_noeof (inp)) << 8;
          if ((c = iobuf_get (inp)) == -1)
            {
              log_error ("%s: 4 byte length invalid\n", iobuf_where (inp));
              rc = gpg_error (GPG_ERR_INV_PACKET);
              goto leave;
            }
          pktlen |= (hdr[hdrlen++] = c);
        }
      else /* Partial body length.  */
        {
          switch (pkttype)
            {
            case PKT_PLAINTEXT:
            case PKT_ENCRYPTED:
            case PKT_ENCRYPTED_MDC:
            case PKT_COMPRESSED:
              iobuf_set_partial_block_mode (inp, c & 0xff);
              pktlen = 0;	/* To indicate partial length.  */
              partial = 1;
              break;

            default:
              log_error ("%s: partial length for invalid"
                         " packet type %d\n", iobuf_where (inp), pkttype);
              rc = gpg_error (GPG_ERR_INV_PACKET);
              goto leave;
            }
        }

    }
  else
    {
      pkttype = (ctb >> 2) & 0xf;
      lenbytes = ((ctb & 3) == 3) ? 0 : (1 << (ctb & 3));
      if (!lenbytes)
	{
	  pktlen = 0;	/* Don't know the value.  */
	  /* This isn't really partial, but we can treat it the same
	     in a "read until the end" sort of way.  */
	  partial = 1;
	  if (pkttype != PKT_ENCRYPTED && pkttype != PKT_PLAINTEXT
	      && pkttype != PKT_COMPRESSED)
	    {
	      log_error ("%s: indeterminate length for invalid"
			 " packet type %d\n", iobuf_where (inp), pkttype);
	      rc = gpg_error (GPG_ERR_INV_PACKET);
	      goto leave;
	    }
	}
      else
	{
	  for (; lenbytes; lenbytes--)
	    {
	      pktlen <<= 8;
	      pktlen |= hdr[hdrlen++] = iobuf_get_noeof (inp);
	    }
	}
    }

  if (pktlen == (unsigned long) (-1))
    {
      /* With some probability this is caused by a problem in the
       * the uncompressing layer - in some error cases it just loops
       * and spits out 0xff bytes. */
      log_error ("%s: garbled packet detected\n", iobuf_where (inp));
      g10_exit (2);
    }

  if (out && pkttype)
    {
      rc = iobuf_write (out, hdr, hdrlen);
      if (!rc)
	rc = copy_packet (inp, out, pkttype, pktlen, partial);
      goto leave;
    }

  if (with_uid && pkttype == PKT_USER_ID)
    ;
  else if (do_skip
	   || !pkttype
	   || (onlykeypkts && pkttype != PKT_PUBLIC_SUBKEY
	       && pkttype != PKT_PUBLIC_KEY
	       && pkttype != PKT_SECRET_SUBKEY && pkttype != PKT_SECRET_KEY))
    {
      iobuf_skip_rest (inp, pktlen, partial);
      *skip = 1;
      rc = 0;
      goto leave;
    }

  if (DBG_PACKET)
    {
#ifdef DEBUG_PARSE_PACKET
      log_debug ("parse_packet(iob=%d): type=%d length=%lu%s (%s.%s.%d)\n",
		 iobuf_id (inp), pkttype, pktlen, new_ctb ? " (new_ctb)" : "",
		 dbg_w, dbg_f, dbg_l);
#else
      log_debug ("parse_packet(iob=%d): type=%d length=%lu%s\n",
		 iobuf_id (inp), pkttype, pktlen,
		 new_ctb ? " (new_ctb)" : "");
#endif
    }

  if (list_mode)
    es_fprintf (listfp, "# off=%lu ctb=%02x tag=%d hlen=%d plen=%lu%s%s\n",
                (unsigned long)pos, ctb, pkttype, hdrlen, pktlen,
                partial? " partial":"",
                new_ctb? " new-ctb":"");

  pkt->pkttype = pkttype;
  rc = GPG_ERR_UNKNOWN_PACKET;	/* default error */
  switch (pkttype)
    {
    case PKT_PUBLIC_KEY:
    case PKT_PUBLIC_SUBKEY:
    case PKT_SECRET_KEY:
    case PKT_SECRET_SUBKEY:
      pkt->pkt.public_key = xmalloc_clear (sizeof *pkt->pkt.public_key);
      rc = parse_key (inp, pkttype, pktlen, hdr, hdrlen, pkt);
      break;
    case PKT_SYMKEY_ENC:
      rc = parse_symkeyenc (inp, pkttype, pktlen, pkt);
      break;
    case PKT_PUBKEY_ENC:
      rc = parse_pubkeyenc (inp, pkttype, pktlen, pkt);
      break;
    case PKT_SIGNATURE:
      pkt->pkt.signature = xmalloc_clear (sizeof *pkt->pkt.signature);
      rc = parse_signature (inp, pkttype, pktlen, pkt->pkt.signature);
      break;
    case PKT_ONEPASS_SIG:
      pkt->pkt.onepass_sig = xmalloc_clear (sizeof *pkt->pkt.onepass_sig);
      rc = parse_onepass_sig (inp, pkttype, pktlen, pkt->pkt.onepass_sig);
      break;
    case PKT_USER_ID:
      rc = parse_user_id (inp, pkttype, pktlen, pkt);
      break;
    case PKT_ATTRIBUTE:
      pkt->pkttype = pkttype = PKT_USER_ID;	/* we store it in the userID */
      rc = parse_attribute (inp, pkttype, pktlen, pkt);
      break;
    case PKT_OLD_COMMENT:
    case PKT_COMMENT:
      rc = parse_comment (inp, pkttype, pktlen, pkt);
      break;
    case PKT_RING_TRUST:
      parse_trust (inp, pkttype, pktlen, pkt);
      rc = 0;
      break;
    case PKT_PLAINTEXT:
      rc = parse_plaintext (inp, pkttype, pktlen, pkt, new_ctb, partial);
      break;
    case PKT_COMPRESSED:
      rc = parse_compressed (inp, pkttype, pktlen, pkt, new_ctb);
      break;
    case PKT_ENCRYPTED:
    case PKT_ENCRYPTED_MDC:
      rc = parse_encrypted (inp, pkttype, pktlen, pkt, new_ctb, partial);
      break;
    case PKT_MDC:
      rc = parse_mdc (inp, pkttype, pktlen, pkt, new_ctb);
      break;
    case PKT_GPG_CONTROL:
      rc = parse_gpg_control (inp, pkttype, pktlen, pkt, partial);
      break;
    case PKT_MARKER:
      rc = parse_marker (inp, pkttype, pktlen);
      break;
    default:
      skip_packet (inp, pkttype, pktlen, partial);
      break;
    }

 leave:
  /* FIXME: Do we leak in case of an error?  */
  if (!rc && iobuf_error (inp))
    rc = GPG_ERR_INV_KEYRING;

  /* FIXME: We use only the error code for now to avoid problems with
     callers which have not been checked to always use gpg_err_code()
     when comparing error codes.  */
  return rc == -1? -1 : gpg_err_code (rc);
}