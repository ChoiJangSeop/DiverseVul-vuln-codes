bgp_capability_parse (struct peer *peer, size_t length, int *mp_capability,
		      u_char **error)
{
  int ret;
  struct stream *s = BGP_INPUT (peer);
  size_t end = stream_get_getp (s) + length;
  
  assert (STREAM_READABLE (s) >= length);
  
  while (stream_get_getp (s) < end)
    {
      size_t start;
      u_char *sp = stream_pnt (s);
      struct capability_header caphdr;
      
      /* We need at least capability code and capability length. */
      if (stream_get_getp(s) + 2 > end)
	{
	  zlog_info ("%s Capability length error (< header)", peer->host);
	  bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
	  return -1;
	}
      
      caphdr.code = stream_getc (s);
      caphdr.length = stream_getc (s);
      start = stream_get_getp (s);
      
      /* Capability length check sanity check. */
      if (start + caphdr.length > end)
	{
	  zlog_info ("%s Capability length error (< length)", peer->host);
	  bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
	  return -1;
	}
      
      if (BGP_DEBUG (normal, NORMAL))
	zlog_debug ("%s OPEN has %s capability (%u), length %u",
		   peer->host,
		   LOOKUP (capcode_str, caphdr.code),
		   caphdr.code, caphdr.length);
      
      /* Length sanity check, type-specific, for known capabilities */
      switch (caphdr.code)
        {
          case CAPABILITY_CODE_MP:
          case CAPABILITY_CODE_REFRESH:
          case CAPABILITY_CODE_REFRESH_OLD:
          case CAPABILITY_CODE_ORF:
          case CAPABILITY_CODE_ORF_OLD:
          case CAPABILITY_CODE_RESTART:
          case CAPABILITY_CODE_AS4:
          case CAPABILITY_CODE_DYNAMIC:
              /* Check length. */
              if (caphdr.length < cap_minsizes[caphdr.code])
                {
                  zlog_info ("%s %s Capability length error: got %u,"
                             " expected at least %u",
                             peer->host, 
                             LOOKUP (capcode_str, caphdr.code),
                             caphdr.length, 
			     (unsigned) cap_minsizes[caphdr.code]);
                  bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
                  return -1;
                }
          /* we deliberately ignore unknown codes, see below */
          default:
            break;
        }
      
      switch (caphdr.code)
        {
          case CAPABILITY_CODE_MP:
            {
	      *mp_capability = 1;

              /* Ignore capability when override-capability is set. */
              if (! CHECK_FLAG (peer->flags, PEER_FLAG_OVERRIDE_CAPABILITY))
                {
                  /* Set negotiated value. */
                  ret = bgp_capability_mp (peer, &caphdr);

                  /* Unsupported Capability. */
                  if (ret < 0)
                    {
                      /* Store return data. */
                      memcpy (*error, sp, caphdr.length + 2);
                      *error += caphdr.length + 2;
                    }
                }
            }
            break;
          case CAPABILITY_CODE_REFRESH:
          case CAPABILITY_CODE_REFRESH_OLD:
            {
              /* BGP refresh capability */
              if (caphdr.code == CAPABILITY_CODE_REFRESH_OLD)
                SET_FLAG (peer->cap, PEER_CAP_REFRESH_OLD_RCV);
              else
                SET_FLAG (peer->cap, PEER_CAP_REFRESH_NEW_RCV);
            }
            break;
          case CAPABILITY_CODE_ORF:
          case CAPABILITY_CODE_ORF_OLD:
            if (bgp_capability_orf (peer, &caphdr))
              return -1;
            break;
          case CAPABILITY_CODE_RESTART:
            if (bgp_capability_restart (peer, &caphdr))
              return -1;
            break;
          case CAPABILITY_CODE_DYNAMIC:
            SET_FLAG (peer->cap, PEER_CAP_DYNAMIC_RCV);
            break;
          case CAPABILITY_CODE_AS4:
              /* Already handled as a special-case parsing of the capabilities
               * at the beginning of OPEN processing. So we care not a jot
               * for the value really, only error case.
               */
              if (!bgp_capability_as4 (peer, &caphdr))
                return -1;
              break;            
          default:
            if (caphdr.code > 128)
              {
                /* We don't send Notification for unknown vendor specific
                   capabilities.  It seems reasonable for now...  */
                zlog_warn ("%s Vendor specific capability %d",
                           peer->host, caphdr.code);
              }
            else
              {
                zlog_warn ("%s unrecognized capability code: %d - ignored",
                           peer->host, caphdr.code);
                memcpy (*error, sp, caphdr.length + 2);
                *error += caphdr.length + 2;
              }
          }
      if (stream_get_getp(s) != (start + caphdr.length))
        {
          if (stream_get_getp(s) > (start + caphdr.length))
            zlog_warn ("%s Cap-parser for %s read past cap-length, %u!",
                       peer->host, LOOKUP (capcode_str, caphdr.code),
                       caphdr.length);
          stream_set_getp (s, start + caphdr.length);
        }
    }
  return 0;
}