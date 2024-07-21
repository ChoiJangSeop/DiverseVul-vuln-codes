bgp_attr_origin (struct peer *peer, bgp_size_t length, 
		 struct attr *attr, u_char flag, u_char *startp)
{
  bgp_size_t total;

  /* total is entire attribute length include Attribute Flags (1),
     Attribute Type code (1) and Attribute length (1 or 2).  */
  total = length + (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN) ? 4 : 3);

  /* If any recognized attribute has Attribute Flags that conflict
     with the Attribute Type Code, then the Error Subcode is set to
     Attribute Flags Error.  The Data field contains the erroneous
     attribute (type, length and value). */
  if (bgp_attr_flag_invalid (peer, BGP_ATTR_ORIGIN, flag))
    return bgp_attr_malformed (peer, BGP_ATTR_ORIGIN, flag,
                               BGP_NOTIFY_UPDATE_ATTR_FLAG_ERR,
                               startp, total);

  /* If any recognized attribute has Attribute Length that conflicts
     with the expected length (based on the attribute type code), then
     the Error Subcode is set to Attribute Length Error.  The Data
     field contains the erroneous attribute (type, length and
     value). */
  if (length != 1)
    {
      zlog (peer->log, LOG_ERR, "Origin attribute length is not one %d",
	    length);
      return bgp_attr_malformed (peer, BGP_ATTR_ORIGIN, flag,
                                 BGP_NOTIFY_UPDATE_ATTR_LENG_ERR,
                                 startp, total);
    }

  /* Fetch origin attribute. */
  attr->origin = stream_getc (BGP_INPUT (peer));

  /* If the ORIGIN attribute has an undefined value, then the Error
     Subcode is set to Invalid Origin Attribute.  The Data field
     contains the unrecognized attribute (type, length and value). */
  if ((attr->origin != BGP_ORIGIN_IGP)
      && (attr->origin != BGP_ORIGIN_EGP)
      && (attr->origin != BGP_ORIGIN_INCOMPLETE))
    {
      zlog (peer->log, LOG_ERR, "Origin attribute value is invalid %d",
	      attr->origin);
      return bgp_attr_malformed (peer, BGP_ATTR_ORIGIN, flag,
                                 BGP_NOTIFY_UPDATE_INVAL_ORIGIN,
                                 startp, total);
    }

  /* Set oring attribute flag. */
  attr->flag |= ATTR_FLAG_BIT (BGP_ATTR_ORIGIN);

  return 0;
}