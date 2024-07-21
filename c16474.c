dns_init()
{
  ip_addr_t dnsserver;

  LWIP_ASSERT("sanity check SIZEOF_DNS_QUERY",
    sizeof(struct dns_query) == SIZEOF_DNS_QUERY);
  LWIP_ASSERT("sanity check SIZEOF_DNS_ANSWER",
    sizeof(struct dns_answer) <= SIZEOF_DNS_ANSWER_ASSERT);

  dns_payload = (u8_t *)LWIP_MEM_ALIGN(dns_payload_buffer);

  /* initialize default DNS server address */
  DNS_SERVER_ADDRESS(&dnsserver);

  LWIP_DEBUGF(DNS_DEBUG, ("dns_init: initializing\n"));

  /* if dns client not yet initialized... */
  if (dns_pcb == NULL) {
    dns_pcb = udp_new();

    if (dns_pcb != NULL) {
      /* initialize DNS table not needed (initialized to zero since it is a
       * global variable) */
      LWIP_ASSERT("For implicit initialization to work, DNS_STATE_UNUSED needs to be 0",
        DNS_STATE_UNUSED == 0);

      /* initialize DNS client */
      udp_bind(dns_pcb, IP_ADDR_ANY, 0);
      udp_recv(dns_pcb, dns_recv, NULL);

      /* initialize default DNS primary server */
      dns_setserver(0, &dnsserver);
    }
  }
#if DNS_LOCAL_HOSTLIST
  dns_init_local();
#endif
}