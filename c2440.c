dump_sig_subpkt (int hashed, int type, int critical,
		 const byte * buffer, size_t buflen, size_t length)
{
  const char *p = NULL;
  int i;

  /* The CERT has warning out with explains how to use GNUPG to detect
   * the ARRs - we print our old message here when it is a faked ARR
   * and add an additional notice.  */
  if (type == SIGSUBPKT_ARR && !hashed)
    {
      es_fprintf (listfp,
                  "\tsubpkt %d len %u (additional recipient request)\n"
                  "WARNING: PGP versions > 5.0 and < 6.5.8 will automagically "
                  "encrypt to this key and thereby reveal the plaintext to "
                  "the owner of this ARR key. Detailed info follows:\n",
                  type, (unsigned) length);
    }

  buffer++;
  length--;

  es_fprintf (listfp, "\t%s%ssubpkt %d len %u (",	/*) */
              critical ? "critical " : "",
              hashed ? "hashed " : "", type, (unsigned) length);
  if (length > buflen)
    {
      es_fprintf (listfp, "too short: buffer is only %u)\n", (unsigned) buflen);
      return;
    }
  switch (type)
    {
    case SIGSUBPKT_SIG_CREATED:
      if (length >= 4)
	es_fprintf (listfp, "sig created %s",
                    strtimestamp (buffer_to_u32 (buffer)));
      break;
    case SIGSUBPKT_SIG_EXPIRE:
      if (length >= 4)
	{
	  if (buffer_to_u32 (buffer))
	    es_fprintf (listfp, "sig expires after %s",
                        strtimevalue (buffer_to_u32 (buffer)));
	  else
	    es_fprintf (listfp, "sig does not expire");
	}
      break;
    case SIGSUBPKT_EXPORTABLE:
      if (length)
	es_fprintf (listfp, "%sexportable", *buffer ? "" : "not ");
      break;
    case SIGSUBPKT_TRUST:
      if (length != 2)
	p = "[invalid trust subpacket]";
      else
	es_fprintf (listfp, "trust signature of depth %d, value %d", buffer[0],
                    buffer[1]);
      break;
    case SIGSUBPKT_REGEXP:
      if (!length)
	p = "[invalid regexp subpacket]";
      else
        {
          es_fprintf (listfp, "regular expression: \"");
          es_write_sanitized (listfp, buffer, length, "\"", NULL);
          p = "\"";
        }
      break;
    case SIGSUBPKT_REVOCABLE:
      if (length)
	es_fprintf (listfp, "%srevocable", *buffer ? "" : "not ");
      break;
    case SIGSUBPKT_KEY_EXPIRE:
      if (length >= 4)
	{
	  if (buffer_to_u32 (buffer))
	    es_fprintf (listfp, "key expires after %s",
                        strtimevalue (buffer_to_u32 (buffer)));
	  else
	    es_fprintf (listfp, "key does not expire");
	}
      break;
    case SIGSUBPKT_PREF_SYM:
      es_fputs ("pref-sym-algos:", listfp);
      for (i = 0; i < length; i++)
	es_fprintf (listfp, " %d", buffer[i]);
      break;
    case SIGSUBPKT_REV_KEY:
      es_fputs ("revocation key: ", listfp);
      if (length < 22)
	p = "[too short]";
      else
	{
	  es_fprintf (listfp, "c=%02x a=%d f=", buffer[0], buffer[1]);
	  for (i = 2; i < length; i++)
	    es_fprintf (listfp, "%02X", buffer[i]);
	}
      break;
    case SIGSUBPKT_ISSUER:
      if (length >= 8)
	es_fprintf (listfp, "issuer key ID %08lX%08lX",
                    (ulong) buffer_to_u32 (buffer),
                    (ulong) buffer_to_u32 (buffer + 4));
      break;
    case SIGSUBPKT_NOTATION:
      {
	es_fputs ("notation: ", listfp);
	if (length < 8)
	  p = "[too short]";
	else
	  {
	    const byte *s = buffer;
	    size_t n1, n2;

	    n1 = (s[4] << 8) | s[5];
	    n2 = (s[6] << 8) | s[7];
	    s += 8;
	    if (8 + n1 + n2 != length)
	      p = "[error]";
	    else
	      {
		es_write_sanitized (listfp, s, n1, ")", NULL);
		es_putc ('=', listfp);

		if (*buffer & 0x80)
		  es_write_sanitized (listfp, s + n1, n2, ")", NULL);
		else
		  p = "[not human readable]";
	      }
	  }
      }
      break;
    case SIGSUBPKT_PREF_HASH:
      es_fputs ("pref-hash-algos:", listfp);
      for (i = 0; i < length; i++)
	es_fprintf (listfp, " %d", buffer[i]);
      break;
    case SIGSUBPKT_PREF_COMPR:
      es_fputs ("pref-zip-algos:", listfp);
      for (i = 0; i < length; i++)
	es_fprintf (listfp, " %d", buffer[i]);
      break;
    case SIGSUBPKT_KS_FLAGS:
      es_fputs ("key server preferences:", listfp);
      for (i = 0; i < length; i++)
	es_fprintf (listfp, " %02X", buffer[i]);
      break;
    case SIGSUBPKT_PREF_KS:
      es_fputs ("preferred key server: ", listfp);
      es_write_sanitized (listfp, buffer, length, ")", NULL);
      break;
    case SIGSUBPKT_PRIMARY_UID:
      p = "primary user ID";
      break;
    case SIGSUBPKT_POLICY:
      es_fputs ("policy: ", listfp);
      es_write_sanitized (listfp, buffer, length, ")", NULL);
      break;
    case SIGSUBPKT_KEY_FLAGS:
      es_fputs ("key flags:", listfp);
      for (i = 0; i < length; i++)
	es_fprintf (listfp, " %02X", buffer[i]);
      break;
    case SIGSUBPKT_SIGNERS_UID:
      p = "signer's user ID";
      break;
    case SIGSUBPKT_REVOC_REASON:
      if (length)
	{
	  es_fprintf (listfp, "revocation reason 0x%02x (", *buffer);
	  es_write_sanitized (listfp, buffer + 1, length - 1, ")", NULL);
	  p = ")";
	}
      break;
    case SIGSUBPKT_ARR:
      es_fputs ("Big Brother's key (ignored): ", listfp);
      if (length < 22)
	p = "[too short]";
      else
	{
	  es_fprintf (listfp, "c=%02x a=%d f=", buffer[0], buffer[1]);
          if (length > 2)
            es_write_hexstring (listfp, buffer+2, length-2, 0, NULL);
	}
      break;
    case SIGSUBPKT_FEATURES:
      es_fputs ("features:", listfp);
      for (i = 0; i < length; i++)
	es_fprintf (listfp, " %02x", buffer[i]);
      break;
    case SIGSUBPKT_SIGNATURE:
      es_fputs ("signature: ", listfp);
      if (length < 17)
	p = "[too short]";
      else
	es_fprintf (listfp, "v%d, class 0x%02X, algo %d, digest algo %d",
                    buffer[0],
                    buffer[0] == 3 ? buffer[2] : buffer[1],
                    buffer[0] == 3 ? buffer[15] : buffer[2],
                    buffer[0] == 3 ? buffer[16] : buffer[3]);
      break;
    default:
      if (type >= 100 && type <= 110)
	p = "experimental / private subpacket";
      else
	p = "?";
      break;
    }

  es_fprintf (listfp, "%s)\n", p ? p : "");
}