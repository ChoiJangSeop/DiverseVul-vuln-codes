dao_input_nonstoring(void)
{
#if RPL_WITH_NON_STORING
  uip_ipaddr_t dao_sender_addr;
  uip_ipaddr_t dao_parent_addr;
  rpl_dag_t *dag;
  rpl_instance_t *instance;
  unsigned char *buffer;
  uint16_t sequence;
  uint8_t instance_id;
  uint8_t lifetime;
  uint8_t prefixlen;
  uint8_t flags;
  uint8_t subopt_type;
  uip_ipaddr_t prefix;
  uint8_t buffer_length;
  int pos;
  int len;
  int i;

  /* Destination Advertisement Object */
  LOG_INFO("Received a DAO from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_("\n");

  prefixlen = 0;

  uip_ipaddr_copy(&dao_sender_addr, &UIP_IP_BUF->srcipaddr);
  memset(&dao_parent_addr, 0, 16);

  buffer = UIP_ICMP_PAYLOAD;
  buffer_length = uip_len - uip_l3_icmp_hdr_len;

  pos = 0;
  instance_id = buffer[pos++];
  instance = rpl_get_instance(instance_id);
  lifetime = instance->default_lifetime;

  flags = buffer[pos++];
  /* reserved */
  pos++;
  sequence = buffer[pos++];

  dag = instance->current_dag;
  /* Is the DAG ID present? */
  if(flags & RPL_DAO_D_FLAG) {
    if(memcmp(&dag->dag_id, &buffer[pos], sizeof(dag->dag_id))) {
      LOG_INFO("Ignoring a DAO for a DAG different from ours\n");
      return;
    }
    pos += 16;
  }

  /* Check if there are any RPL options present. */
  for(i = pos; i < buffer_length; i += len) {
    subopt_type = buffer[i];
    if(subopt_type == RPL_OPTION_PAD1) {
      len = 1;
    } else {
      /* The option consists of a two-byte header and a payload. */
      len = 2 + buffer[i + 1];
    }

    switch(subopt_type) {
      case RPL_OPTION_TARGET:
        /* Handle the target option. */
        prefixlen = buffer[i + 3];
        memset(&prefix, 0, sizeof(prefix));
        memcpy(&prefix, buffer + i + 4, (prefixlen + 7) / CHAR_BIT);
        break;
      case RPL_OPTION_TRANSIT:
        /* The path sequence and control are ignored. */
        /*      pathcontrol = buffer[i + 3];
                pathsequence = buffer[i + 4];*/
        lifetime = buffer[i + 5];
        if(len >= 20) {
          memcpy(&dao_parent_addr, buffer + i + 6, 16);
        }
        break;
    }
  }

  LOG_INFO("DAO lifetime: %u, prefix length: %u prefix: ",
         (unsigned)lifetime, (unsigned)prefixlen);
  LOG_INFO_6ADDR(&prefix);
  LOG_INFO_(", parent: ");
  LOG_INFO_6ADDR(&dao_parent_addr);
  LOG_INFO_("\n");

  if(lifetime == RPL_ZERO_LIFETIME) {
    LOG_DBG("No-Path DAO received\n");
    uip_sr_expire_parent(dag, &prefix, &dao_parent_addr);
  } else {
    if(uip_sr_update_node(dag, &prefix, &dao_parent_addr, RPL_LIFETIME(instance, lifetime)) == NULL) {
      LOG_WARN("DAO failed to add link prefix: ");
      LOG_WARN_6ADDR(&prefix);
      LOG_WARN_(", parent: ");
      LOG_WARN_6ADDR(&dao_parent_addr);
      LOG_WARN_("\n");
      return;
    }
  }

  if(flags & RPL_DAO_K_FLAG) {
    LOG_DBG("Sending DAO ACK\n");
    uipbuf_clear();
    dao_ack_output(instance, &dao_sender_addr, sequence,
                   RPL_DAO_ACK_UNCONDITIONAL_ACCEPT);
  }
#endif /* RPL_WITH_NON_STORING */
}