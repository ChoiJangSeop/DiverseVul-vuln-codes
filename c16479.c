dns_call_found(u8_t idx, ip_addr_t* addr)
{
#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING) != 0)
  u8_t i;
  for (i = 0; i < DNS_MAX_REQUESTS; i++) {
    if (dns_requests[i].found && (dns_requests[i].dns_table_idx == idx)) {
      (*dns_requests[i].found)(dns_table[idx].name, addr, dns_requests[i].arg);
      /* flush this entry */
      dns_requests[i].found   = NULL;
    }
  }
#else
  if (dns_requests[idx].found) {
    (*dns_requests[idx].found)(dns_table[idx].name, addr, dns_requests[idx].arg);
  }
  dns_requests[idx].found = NULL;
#endif
}