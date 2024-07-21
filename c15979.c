dio_input(void)
{
  unsigned char *buffer;
  uint8_t buffer_length;
  rpl_dio_t dio;
  uint8_t subopt_type;
  int i;
  int len;
  uip_ipaddr_t from;

  memset(&dio, 0, sizeof(dio));

  /* Set default values in case the DIO configuration option is missing. */
  dio.dag_intdoubl = RPL_DIO_INTERVAL_DOUBLINGS;
  dio.dag_intmin = RPL_DIO_INTERVAL_MIN;
  dio.dag_redund = RPL_DIO_REDUNDANCY;
  dio.dag_min_hoprankinc = RPL_MIN_HOPRANKINC;
  dio.dag_max_rankinc = RPL_MAX_RANKINC;
  dio.ocp = RPL_OF_OCP;
  dio.default_lifetime = RPL_DEFAULT_LIFETIME;
  dio.lifetime_unit = RPL_DEFAULT_LIFETIME_UNIT;

  uip_ipaddr_copy(&from, &UIP_IP_BUF->srcipaddr);

  /* DAG Information Object */
  LOG_INFO("Received a DIO from ");
  LOG_INFO_6ADDR(&from);
  LOG_INFO_("\n");

  buffer_length = uip_len - uip_l3_icmp_hdr_len;

  /* Process the DIO base option. */
  i = 0;
  buffer = UIP_ICMP_PAYLOAD;

  dio.instance_id = buffer[i++];
  dio.version = buffer[i++];
  dio.rank = get16(buffer, i);
  i += 2;

  LOG_DBG("Incoming DIO (id, ver, rank) = (%u,%u,%u)\n",
         (unsigned)dio.instance_id,
         (unsigned)dio.version,
         (unsigned)dio.rank);

  dio.grounded = buffer[i] & RPL_DIO_GROUNDED;
  dio.mop = (buffer[i]& RPL_DIO_MOP_MASK) >> RPL_DIO_MOP_SHIFT;
  dio.preference = buffer[i++] & RPL_DIO_PREFERENCE_MASK;

  dio.dtsn = buffer[i++];
  /* two reserved bytes */
  i += 2;

  memcpy(&dio.dag_id, buffer + i, sizeof(dio.dag_id));
  i += sizeof(dio.dag_id);

  LOG_DBG("Incoming DIO (dag_id, pref) = (");
  LOG_DBG_6ADDR(&dio.dag_id);
  LOG_DBG_(", %u)\n", dio.preference);

  /* Check if there are any DIO suboptions. */
  for(; i < buffer_length; i += len) {
    subopt_type = buffer[i];
    if(subopt_type == RPL_OPTION_PAD1) {
      len = 1;
    } else {
      /* Suboption with a two-byte header + payload */
      len = 2 + buffer[i + 1];
    }

    if(len + i > buffer_length) {
      LOG_WARN("Invalid DIO packet\n");
      RPL_STAT(rpl_stats.malformed_msgs++);
      goto discard;
    }

    LOG_DBG("Incoming DIO (option, length) = (%u, %u)\n", subopt_type, len - 2);

    switch(subopt_type) {
      case RPL_OPTION_DAG_METRIC_CONTAINER:
        if(len < 6) {
          LOG_WARN("Invalid DAG MC, len = %d\n", len);
          RPL_STAT(rpl_stats.malformed_msgs++);
          goto discard;
        }
        dio.mc.type = buffer[i + 2];
        dio.mc.flags = buffer[i + 3] << 1;
        dio.mc.flags |= buffer[i + 4] >> 7;
        dio.mc.aggr = (buffer[i + 4] >> 4) & 0x3;
        dio.mc.prec = buffer[i + 4] & 0xf;
        dio.mc.length = buffer[i + 5];

        if(dio.mc.type == RPL_DAG_MC_NONE) {
          /* No metric container: do nothing */
        } else if(dio.mc.type == RPL_DAG_MC_ETX) {
          dio.mc.obj.etx = get16(buffer, i + 6);

          LOG_DBG("DAG MC: type %u, flags %u, aggr %u, prec %u, length %u, ETX %u\n",
                 (unsigned)dio.mc.type,
                 (unsigned)dio.mc.flags,
                 (unsigned)dio.mc.aggr,
                 (unsigned)dio.mc.prec,
                 (unsigned)dio.mc.length,
                 (unsigned)dio.mc.obj.etx);
        } else if(dio.mc.type == RPL_DAG_MC_ENERGY) {
          dio.mc.obj.energy.flags = buffer[i + 6];
          dio.mc.obj.energy.energy_est = buffer[i + 7];
        } else {
          LOG_WARN("Unhandled DAG MC type: %u\n", (unsigned)dio.mc.type);
          goto discard;
        }
        break;
      case RPL_OPTION_ROUTE_INFO:
        if(len < 9) {
          LOG_WARN("Invalid destination prefix option, len = %d\n", len);
          RPL_STAT(rpl_stats.malformed_msgs++);
          goto discard;
        }

        /* The flags field includes the preference value. */
        dio.destination_prefix.length = buffer[i + 2];
        dio.destination_prefix.flags = buffer[i + 3];
        dio.destination_prefix.lifetime = get32(buffer, i + 4);

        if(((dio.destination_prefix.length + 7) / 8) + 8 <= len &&
           dio.destination_prefix.length <= 128) {
          LOG_INFO("Copying destination prefix\n");
          memcpy(&dio.destination_prefix.prefix, &buffer[i + 8],
                 (dio.destination_prefix.length + 7) / 8);
        } else {
          LOG_WARN("Invalid route info option, len = %d\n", len);
          RPL_STAT(rpl_stats.malformed_msgs++);
          goto discard;
        }

        break;
      case RPL_OPTION_DAG_CONF:
        if(len != 16) {
          LOG_WARN("Invalid DAG configuration option, len = %d\n", len);
          RPL_STAT(rpl_stats.malformed_msgs++);
          goto discard;
        }

        /* Path control field not yet implemented - at i + 2 */
        dio.dag_intdoubl = buffer[i + 3];
        dio.dag_intmin = buffer[i + 4];
        dio.dag_redund = buffer[i + 5];
        dio.dag_max_rankinc = get16(buffer, i + 6);
        dio.dag_min_hoprankinc = get16(buffer, i + 8);
        dio.ocp = get16(buffer, i + 10);
        /* buffer + 12 is reserved */
        dio.default_lifetime = buffer[i + 13];
        dio.lifetime_unit = get16(buffer, i + 14);
        LOG_INFO("DAG conf:dbl=%d, min=%d red=%d maxinc=%d mininc=%d ocp=%d d_l=%u l_u=%u\n",
               dio.dag_intdoubl, dio.dag_intmin, dio.dag_redund,
               dio.dag_max_rankinc, dio.dag_min_hoprankinc, dio.ocp,
               dio.default_lifetime, dio.lifetime_unit);
        break;
      case RPL_OPTION_PREFIX_INFO:
        if(len != 32) {
          LOG_WARN("Invalid DAG prefix info, len != 32\n");
          RPL_STAT(rpl_stats.malformed_msgs++);
          goto discard;
        }
        dio.prefix_info.length = buffer[i + 2];
        dio.prefix_info.flags = buffer[i + 3];
        /* valid lifetime is ingnored for now - at i + 4 */
        /* preferred lifetime stored in lifetime */
        dio.prefix_info.lifetime = get32(buffer, i + 8);
        /* 32-bit reserved at i + 12 */
        LOG_INFO("Copying prefix information\n");
        memcpy(&dio.prefix_info.prefix, &buffer[i + 16], 16);
        break;
      default:
        LOG_WARN("Unsupported suboption type in DIO: %u\n",
               (unsigned)subopt_type);
    }
  }

#ifdef RPL_DEBUG_DIO_INPUT
  RPL_DEBUG_DIO_INPUT(&from, &dio);
#endif

  rpl_process_dio(&from, &dio);

discard:
  uipbuf_clear();
}