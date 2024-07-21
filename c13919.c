static int extractRDNSequence(struct ndpi_packet_struct *packet,
			      u_int offset, char *buffer, u_int buffer_len,
			      char *rdnSeqBuf, u_int *rdnSeqBuf_offset,
			      u_int rdnSeqBuf_len,
			      const char *label) {
  u_int8_t str_len = packet->payload[offset+4], is_printable = 1;
  char *str;
  u_int len, j;

  // packet is truncated... further inspection is not needed
  if((offset+4+str_len) >= packet->payload_packet_len)
    return(-1);

  str = (char*)&packet->payload[offset+5];

  len = (u_int)ndpi_min(str_len, buffer_len-1);
  strncpy(buffer, str, len);
  buffer[len] = '\0';

  // check string is printable
  for(j = 0; j < len; j++) {
    if(!ndpi_isprint(buffer[j])) {
      is_printable = 0;
      break;
    }
  }

  if(is_printable) {
    int rc = snprintf(&rdnSeqBuf[*rdnSeqBuf_offset],
		      rdnSeqBuf_len-(*rdnSeqBuf_offset),
		      "%s%s=%s", (*rdnSeqBuf_offset > 0) ? ", " : "",
		      label, buffer);

    if(rc > 0)
      (*rdnSeqBuf_offset) += rc;
  }

  return(is_printable);
}