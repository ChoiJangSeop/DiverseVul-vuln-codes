bgp_attr_flag_invalid (struct peer *peer, u_int8_t attr_code, u_int8_t flags)
{
  u_int8_t mask = BGP_ATTR_FLAG_EXTLEN;
  
  /* there may be attributes we don't know about */
  if (attr_code > attr_flags_values_max)
    return 0;
  if (attr_flags_values[attr_code] == 0)
    return 0;
  
  /* RFC4271, "For well-known attributes, the Transitive bit MUST be set to
   * 1."
   */
  if (!CHECK_FLAG (BGP_ATTR_FLAG_OPTIONAL, flags)
      && !CHECK_FLAG (BGP_ATTR_FLAG_TRANS, flags))
    {
      zlog (peer->log, LOG_ERR,
            "%s well-known attributes must have transitive flag set (%x)",
            LOOKUP (attr_str, attr_code), flags);
      return 1;
    }
  
  /* "For well-known attributes and for optional non-transitive attributes,
   *  the Partial bit MUST be set to 0." 
   */
  if (CHECK_FLAG (flags, BGP_ATTR_FLAG_PARTIAL))
    {
      if (!CHECK_FLAG (flags, BGP_ATTR_FLAG_OPTIONAL))
        {
          zlog (peer->log, LOG_ERR,
                "%s well-known attribute "
                "must NOT have the partial flag set (%x)",
                 LOOKUP (attr_str, attr_code), flags);
          return 1;
        }
      if (CHECK_FLAG (flags, BGP_ATTR_FLAG_OPTIONAL)
          && !CHECK_FLAG (flags, BGP_ATTR_FLAG_TRANS))
        {
          zlog (peer->log, LOG_ERR,
                "%s optional + transitive attribute "
                "must NOT have the partial flag set (%x)",
                 LOOKUP (attr_str, attr_code), flags);
          return 1;
        }
    }
  
  /* Optional transitive attributes may go through speakers that don't
   * reocgnise them and set the Partial bit.
   */
  if (CHECK_FLAG (flags, BGP_ATTR_FLAG_OPTIONAL)
      && CHECK_FLAG (flags, BGP_ATTR_FLAG_TRANS))
    SET_FLAG (mask, BGP_ATTR_FLAG_PARTIAL);
  
  if ((flags & ~attr_flags_values[attr_code])
      == attr_flags_values[attr_code])
    return 0;
  
  bgp_attr_flags_diagnose (peer, attr_code, attr_flags_values[attr_code],
                           flags);
  return 1;
}