dns_tmr(void)
{
  if (dns_pcb != NULL) {
    LWIP_DEBUGF(DNS_DEBUG, ("dns_tmr: dns_check_entries\n"));
    dns_check_entries();
  }
}