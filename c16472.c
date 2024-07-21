dns_gethostbyname(const char *hostname, ip_addr_t *addr, dns_found_callback found,
                  void *callback_arg)
{
  u32_t ipaddr;
  size_t hostnamelen;
  /* not initialized or no valid server yet, or invalid addr pointer
   * or invalid hostname or invalid hostname length */
  if ((dns_pcb == NULL) || (addr == NULL) ||
      (!hostname) || (!hostname[0])) {
    return ERR_ARG;
  }
  hostnamelen = strlen(hostname);
  if (hostnamelen >= DNS_MAX_NAME_LENGTH) {
    return ERR_ARG;
  }


#if LWIP_HAVE_LOOPIF
  if (strcmp(hostname, "localhost") == 0) {
    ip_addr_set_loopback(addr);
    return ERR_OK;
  }
#endif /* LWIP_HAVE_LOOPIF */

  /* host name already in octet notation? set ip addr and return ERR_OK */
  ipaddr = ipaddr_addr(hostname);
  if (ipaddr == IPADDR_NONE) {
    /* already have this address cached? */
    ipaddr = dns_lookup(hostname);
  }
  if (ipaddr != IPADDR_NONE) {
    ip4_addr_set_u32(addr, ipaddr);
    return ERR_OK;
  }

  /* queue query with specified callback */
  return dns_enqueue(hostname, hostnamelen, found, callback_arg);
}