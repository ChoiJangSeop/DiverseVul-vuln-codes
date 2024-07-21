build_sig_subpkt (PKT_signature *sig, sigsubpkttype_t type,
		  const byte *buffer, size_t buflen )
{
    byte *p;
    int critical, hashed;
    subpktarea_t *oldarea, *newarea;
    size_t nlen, n, n0;

    critical = (type & SIGSUBPKT_FLAG_CRITICAL);
    type &= ~SIGSUBPKT_FLAG_CRITICAL;

    /* Sanity check buffer sizes */
    if(parse_one_sig_subpkt(buffer,buflen,type)<0)
      BUG();

    switch(type)
      {
      case SIGSUBPKT_NOTATION:
      case SIGSUBPKT_POLICY:
      case SIGSUBPKT_REV_KEY:
      case SIGSUBPKT_SIGNATURE:
	/* we do allow multiple subpackets */
	break;

      default:
	/* we don't allow multiple subpackets */
	delete_sig_subpkt(sig->hashed,type);
	delete_sig_subpkt(sig->unhashed,type);
	break;
      }

    /* Any special magic that needs to be done for this type so the
       packet doesn't need to be reparsed? */
    switch(type)
      {
      case SIGSUBPKT_NOTATION:
	sig->flags.notation=1;
	break;

      case SIGSUBPKT_POLICY:
	sig->flags.policy_url=1;
	break;

      case SIGSUBPKT_PREF_KS:
	sig->flags.pref_ks=1;
	break;

      case SIGSUBPKT_EXPORTABLE:
	if(buffer[0])
	  sig->flags.exportable=1;
	else
	  sig->flags.exportable=0;
	break;

      case SIGSUBPKT_REVOCABLE:
	if(buffer[0])
	  sig->flags.revocable=1;
	else
	  sig->flags.revocable=0;
	break;

      case SIGSUBPKT_TRUST:
	sig->trust_depth=buffer[0];
	sig->trust_value=buffer[1];
	break;

      case SIGSUBPKT_REGEXP:
	sig->trust_regexp=buffer;
	break;

	/* This should never happen since we don't currently allow
	   creating such a subpacket, but just in case... */
      case SIGSUBPKT_SIG_EXPIRE:
	if(buffer_to_u32(buffer)+sig->timestamp<=make_timestamp())
	  sig->flags.expired=1;
	else
	  sig->flags.expired=0;
	break;

      default:
	break;
      }

    if( (buflen+1) >= 8384 )
	nlen = 5; /* write 5 byte length header */
    else if( (buflen+1) >= 192 )
	nlen = 2; /* write 2 byte length header */
    else
	nlen = 1; /* just a 1 byte length header */

    switch( type )
      {
	/* The issuer being unhashed is a historical oddity.  It
	   should work equally as well hashed.  Of course, if even an
	   unhashed issuer is tampered with, it makes it awfully hard
	   to verify the sig... */
      case SIGSUBPKT_ISSUER:
      case SIGSUBPKT_SIGNATURE:
        hashed = 0;
        break;
      default:
        hashed = 1;
        break;
      }

    if( critical )
	type |= SIGSUBPKT_FLAG_CRITICAL;

    oldarea = hashed? sig->hashed : sig->unhashed;

    /* Calculate new size of the area and allocate */
    n0 = oldarea? oldarea->len : 0;
    n = n0 + nlen + 1 + buflen; /* length, type, buffer */
    if (oldarea && n <= oldarea->size) { /* fits into the unused space */
        newarea = oldarea;
        /*log_debug ("updating area for type %d\n", type );*/
    }
    else if (oldarea) {
        newarea = xrealloc (oldarea, sizeof (*newarea) + n - 1);
        newarea->size = n;
        /*log_debug ("reallocating area for type %d\n", type );*/
    }
    else {
        newarea = xmalloc (sizeof (*newarea) + n - 1);
        newarea->size = n;
        /*log_debug ("allocating area for type %d\n", type );*/
    }
    newarea->len = n;

    p = newarea->data + n0;
    if (nlen == 5) {
	*p++ = 255;
	*p++ = (buflen+1) >> 24;
	*p++ = (buflen+1) >> 16;
	*p++ = (buflen+1) >>  8;
	*p++ = (buflen+1);
	*p++ = type;
	memcpy (p, buffer, buflen);
    }
    else if (nlen == 2) {
	*p++ = (buflen+1-192) / 256 + 192;
	*p++ = (buflen+1-192) % 256;
	*p++ = type;
	memcpy (p, buffer, buflen);
    }
    else {
	*p++ = buflen+1;
	*p++ = type;
	memcpy (p, buffer, buflen);
    }

    if (hashed)
	sig->hashed = newarea;
    else
	sig->unhashed = newarea;
}