dns_setserver(u8_t numdns, ip_addr_t *dnsserver)
{
  if ((numdns < DNS_MAX_SERVERS) && (dns_pcb != NULL) &&
      (dnsserver != NULL) && !ip_addr_isany(dnsserver)) {
    dns_servers[numdns] = (*dnsserver);
  }
}