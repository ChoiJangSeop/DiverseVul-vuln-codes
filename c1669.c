bgp_attr_munge_as4_attrs (struct peer *peer, struct attr *attr, u_char flag,
                          struct aspath *as4_path, as_t as4_aggregator,
                          struct in_addr *as4_aggregator_addr)
{
  int ignore_as4_path = 0;
  struct aspath *newpath;
  struct attr_extra *attre = attr->extra;
    
  if (CHECK_FLAG (peer->cap, PEER_CAP_AS4_RCV))
    {
      /* peer can do AS4, so we ignore AS4_PATH and AS4_AGGREGATOR
       * if given.
       * It is worth a warning though, because the peer really
       * should not send them
       */
      if (BGP_DEBUG(as4, AS4))
        {
          if (attr->flag & (ATTR_FLAG_BIT(BGP_ATTR_AS4_PATH)))
            zlog_debug ("[AS4] %s %s AS4_PATH",
                        peer->host, "AS4 capable peer, yet it sent");
          
          if (attr->flag & (ATTR_FLAG_BIT(BGP_ATTR_AS4_AGGREGATOR)))
            zlog_debug ("[AS4] %s %s AS4_AGGREGATOR",
                        peer->host, "AS4 capable peer, yet it sent");
        }
      
      return BGP_ATTR_PARSE_PROCEED;
    }
  
  if (attr->flag & (ATTR_FLAG_BIT (BGP_ATTR_AS4_PATH))
      && !(attr->flag & (ATTR_FLAG_BIT (BGP_ATTR_AS_PATH))))
    {
      /* Hu? This is not supposed to happen at all!
       * got as4_path and no aspath,
       *   This should already
       *   have been handled by 'well known attributes missing'
       *   But... yeah, paranoia
       * Take this as a "malformed attribute"
       */
      zlog (peer->log, LOG_ERR, 
            "%s BGP not AS4 capable peer sent AS4_PATH but"
            " no AS_PATH, cant do anything here", peer->host);
      return bgp_attr_malformed (peer, BGP_ATTR_AS_PATH, flag,
                                 BGP_NOTIFY_UPDATE_MAL_ATTR,
                                 NULL, 0);
    }

  /* We have a asn16 peer.  First, look for AS4_AGGREGATOR
   * because that may override AS4_PATH
   */
  if (attr->flag & (ATTR_FLAG_BIT (BGP_ATTR_AS4_AGGREGATOR) ) )
    {
      if (attr->flag & (ATTR_FLAG_BIT (BGP_ATTR_AGGREGATOR) ) )
        {
          assert (attre);
          
          /* received both.
           * if the as_number in aggregator is not AS_TRANS,
           *  then AS4_AGGREGATOR and AS4_PATH shall be ignored
           *        and the Aggregator shall be taken as 
           *        info on the aggregating node, and the AS_PATH
           *        shall be taken as the AS_PATH
           *  otherwise
           *        the Aggregator shall be ignored and the
           *        AS4_AGGREGATOR shall be taken as the
           *        Aggregating node and the AS_PATH is to be
           *        constructed "as in all other cases"
           */
          if (attre->aggregator_as != BGP_AS_TRANS)
            {
              /* ignore */
              if ( BGP_DEBUG(as4, AS4))
                zlog_debug ("[AS4] %s BGP not AS4 capable peer" 
                            " send AGGREGATOR != AS_TRANS and"
                            " AS4_AGGREGATOR, so ignore"
                            " AS4_AGGREGATOR and AS4_PATH", peer->host);
              ignore_as4_path = 1;
            }
          else
            {
              /* "New_aggregator shall be taken as aggregator" */
              attre->aggregator_as = as4_aggregator;
              attre->aggregator_addr.s_addr = as4_aggregator_addr->s_addr;
            }
        }
      else
        {
          /* We received a AS4_AGGREGATOR but no AGGREGATOR.
           * That is bogus - but reading the conditions
           * we have to handle AS4_AGGREGATOR as if it were
           * AGGREGATOR in that case
           */
          if ( BGP_DEBUG(as4, AS4))
            zlog_debug ("[AS4] %s BGP not AS4 capable peer send"
                        " AS4_AGGREGATOR but no AGGREGATOR, will take"
                        " it as if AGGREGATOR with AS_TRANS had been there", peer->host);
          (attre = bgp_attr_extra_get (attr))->aggregator_as = as4_aggregator;
          /* sweep it under the carpet and simulate a "good" AGGREGATOR */
          attr->flag |= (ATTR_FLAG_BIT (BGP_ATTR_AGGREGATOR));
        }
    }

  /* need to reconcile NEW_AS_PATH and AS_PATH */
  if (!ignore_as4_path && (attr->flag & (ATTR_FLAG_BIT( BGP_ATTR_AS4_PATH))))
    {
       newpath = aspath_reconcile_as4 (attr->aspath, as4_path);
       aspath_unintern (&attr->aspath);
       attr->aspath = aspath_intern (newpath);
    }
  return BGP_ATTR_PARSE_PROCEED;
}