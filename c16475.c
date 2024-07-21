dns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port)
{
  u8_t i, entry_idx = DNS_TABLE_SIZE;
  u16_t txid;
  char *ptr;
  struct dns_hdr *hdr;
  struct dns_answer ans;
  struct dns_query qry;
  u16_t nquestions, nanswers;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(port);

  /* is the dns message too big ? */
  if (p->tot_len > DNS_MSG_SIZE) {
    LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: pbuf too big\n"));
    /* free pbuf and return */
    goto memerr;
  }

  /* is the dns message big enough ? */
  if (p->tot_len < (SIZEOF_DNS_HDR + SIZEOF_DNS_QUERY + SIZEOF_DNS_ANSWER)) {
    LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: pbuf too small\n"));
    /* free pbuf and return */
    goto memerr;
  }

  /* copy dns payload inside static buffer for processing */
  if (pbuf_copy_partial(p, dns_payload, p->tot_len, 0) == p->tot_len) {
    /* Match the ID in the DNS header with the name table. */
    hdr = (struct dns_hdr*)dns_payload;
    txid = htons(hdr->id);
    for (i = 0; i < DNS_TABLE_SIZE; i++) {
      struct dns_table_entry *entry = &dns_table[i];
      entry_idx = i;
      if ((entry->state == DNS_STATE_ASKING) &&
          (entry->txid == txid)) {
        /* This entry is now completed. */
        entry->state = DNS_STATE_DONE;
        entry->err   = hdr->flags2 & DNS_FLAG2_ERR_MASK;

        /* We only care about the question(s) and the answers. The authrr
           and the extrarr are simply discarded. */
        nquestions = htons(hdr->numquestions);
        nanswers   = htons(hdr->numanswers);

        /* Check for error. If so, call callback to inform. */
        if (((hdr->flags1 & DNS_FLAG1_RESPONSE) == 0) || (entry->err != 0) || (nquestions != 1)) {
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": error in flags\n", entry->name));
          /* call callback to indicate error, clean up memory and return */
          goto responseerr;
        }

        /* Check whether response comes from the same network address to which the
           question was sent. (RFC 5452) */
        if (!ip_addr_cmp(addr, &dns_servers[entry->numdns])) {
          /* call callback to indicate error, clean up memory and return */
          goto responseerr;
        }

        /* Check if the name in the "question" part match with the name in the entry and
           skip it if equal. */
        ptr = dns_compare_name(entry->name, (char*)dns_payload + SIZEOF_DNS_HDR);
        if (ptr == NULL) {
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": response not match to query\n", entry->name));
          /* call callback to indicate error, clean up memory and return */
          goto responseerr;
        }

        /* check if "question" part matches the request */
        SMEMCPY(&qry, ptr, SIZEOF_DNS_QUERY);
        if((qry.type != PP_HTONS(DNS_RRTYPE_A)) || (qry.cls != PP_HTONS(DNS_RRCLASS_IN))) {
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": response not match to query\n", entry->name));
          /* call callback to indicate error, clean up memory and return */
          goto responseerr;
        }
        /* skip the rest of the "question" part */
        ptr += SIZEOF_DNS_QUERY;

        while (nanswers > 0) {
          /* skip answer resource record's host name */
          ptr = dns_parse_name(ptr);

          /* Check for IP address type and Internet class. Others are discarded. */
          SMEMCPY(&ans, ptr, SIZEOF_DNS_ANSWER);
          if((ans.type == PP_HTONS(DNS_RRTYPE_A)) && (ans.cls == PP_HTONS(DNS_RRCLASS_IN)) &&
             (ans.len == PP_HTONS(sizeof(ip_addr_t))) ) {
            /* read the answer resource record's TTL, and maximize it if needed */
            entry->ttl = ntohl(ans.ttl);
            if (entry->ttl > DNS_MAX_TTL) {
              entry->ttl = DNS_MAX_TTL;
            }
            /* read the IP address after answer resource record's header */
            SMEMCPY(&(entry->ipaddr), (ptr + SIZEOF_DNS_ANSWER), sizeof(ip_addr_t));
            LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": response = ", entry->name));
            ip_addr_debug_print(DNS_DEBUG, (&(entry->ipaddr)));
            LWIP_DEBUGF(DNS_DEBUG, ("\n"));
            /* call specified callback function if provided */
            dns_call_found(entry_idx, &entry->ipaddr);
            if (entry->ttl == 0) {
              /* RFC 883, page 29: "Zero values are
                 interpreted to mean that the RR can only be used for the
                 transaction in progress, and should not be cached."
                 -> flush this entry now */
              goto flushentry;
            }
            /* deallocate memory and return */
            goto memerr;
          } else {
            ptr = ptr + SIZEOF_DNS_ANSWER + htons(ans.len);
          }
          --nanswers;
        }
        LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": error in response\n", entry->name));
        /* call callback to indicate error, clean up memory and return */
        goto responseerr;
      }
    }
  }

  /* deallocate memory and return */
  goto memerr;

responseerr:
  /* ERROR: call specified callback function with NULL as name to indicate an error */
  dns_call_found(entry_idx, NULL);
flushentry:
  /* flush this entry */
  dns_table[entry_idx].state = DNS_STATE_UNUSED;

memerr:
  /* free pbuf */
  pbuf_free(p);
  return;
}