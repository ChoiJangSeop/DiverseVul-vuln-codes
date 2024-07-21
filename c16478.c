dns_check_entry(u8_t i)
{
  err_t err;
  struct dns_table_entry *entry = &dns_table[i];

  LWIP_ASSERT("array index out of bounds", i < DNS_TABLE_SIZE);

  switch (entry->state) {

    case DNS_STATE_NEW: {
      u16_t txid;
      /* initialize new entry */
      txid = dns_create_txid();
      entry->txid = txid;
      entry->state   = DNS_STATE_ASKING;
      entry->numdns  = 0;
      entry->tmr     = 1;
      entry->retries = 0;

      /* send DNS packet for this entry */
      err = dns_send(entry->numdns, entry->name, txid);
      if (err != ERR_OK) {
        LWIP_DEBUGF(DNS_DEBUG | LWIP_DBG_LEVEL_WARNING,
                    ("dns_send returned error: %s\n", lwip_strerr(err)));
      }
      break;
    }

    case DNS_STATE_ASKING:
      if (--entry->tmr == 0) {
        if (++entry->retries == DNS_MAX_RETRIES) {
          if ((entry->numdns+1<DNS_MAX_SERVERS) && !ip_addr_isany(&dns_servers[entry->numdns+1])) {
            /* change of server */
            entry->numdns++;
            entry->tmr     = 1;
            entry->retries = 0;
            break;
          } else {
            LWIP_DEBUGF(DNS_DEBUG, ("dns_check_entry: \"%s\": timeout\n", entry->name));
            /* call specified callback function if provided */
            dns_call_found(i, NULL);
            /* flush this entry */
            entry->state   = DNS_STATE_UNUSED;
            break;
          }
        }

        /* wait longer for the next retry */
        entry->tmr = entry->retries;

        /* send DNS packet for this entry */
        err = dns_send(entry->numdns, entry->name, entry->txid);
        if (err != ERR_OK) {
          LWIP_DEBUGF(DNS_DEBUG | LWIP_DBG_LEVEL_WARNING,
                      ("dns_send returned error: %s\n", lwip_strerr(err)));
        }
      }
      break;
    case DNS_STATE_DONE:
      /* if the time to live is nul */
      if ((entry->ttl == 0) || (--entry->ttl == 0)) {
        LWIP_DEBUGF(DNS_DEBUG, ("dns_check_entry: \"%s\": flush\n", entry->name));
        /* flush this entry, there cannot be any related pending entries in this state */
        entry->state = DNS_STATE_UNUSED;
      }
      break;
    case DNS_STATE_UNUSED:
      /* nothing to do */
      break;
    default:
      LWIP_ASSERT("unknown dns_table entry state:", 0);
      break;
  }
}