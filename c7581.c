MG_INTERNAL int parse_mqtt(struct mbuf *io, struct mg_mqtt_message *mm) {
  uint8_t header;
  size_t len = 0, len_len = 0;
  const char *p, *end;
  unsigned char lc = 0;
  int cmd;

  if (io->len < 2) return MG_MQTT_ERROR_INCOMPLETE_MSG;
  header = io->buf[0];
  cmd = header >> 4;

  /* decode mqtt variable length */
  len = len_len = 0;
  p = io->buf + 1;
  while ((size_t)(p - io->buf) < io->len) {
    lc = *((const unsigned char *) p++);
    len += (lc & 0x7f) << 7 * len_len;
    len_len++;
    if (!(lc & 0x80)) break;
    if (len_len > 4) return MG_MQTT_ERROR_MALFORMED_MSG;
  }

  end = p + len;
  if (lc & 0x80 || len > (io->len - (p - io->buf))) {
    return MG_MQTT_ERROR_INCOMPLETE_MSG;
  }

  mm->cmd = cmd;
  mm->qos = MG_MQTT_GET_QOS(header);

  switch (cmd) {
    case MG_MQTT_CMD_CONNECT: {
      p = scanto(p, &mm->protocol_name);
      if (p > end - 4) return MG_MQTT_ERROR_MALFORMED_MSG;
      mm->protocol_version = *(uint8_t *) p++;
      mm->connect_flags = *(uint8_t *) p++;
      mm->keep_alive_timer = getu16(p);
      p += 2;
      if (p >= end) return MG_MQTT_ERROR_MALFORMED_MSG;
      p = scanto(p, &mm->client_id);
      if (p > end) return MG_MQTT_ERROR_MALFORMED_MSG;
      if (mm->connect_flags & MG_MQTT_HAS_WILL) {
        if (p >= end) return MG_MQTT_ERROR_MALFORMED_MSG;
        p = scanto(p, &mm->will_topic);
      }
      if (mm->connect_flags & MG_MQTT_HAS_WILL) {
        if (p >= end) return MG_MQTT_ERROR_MALFORMED_MSG;
        p = scanto(p, &mm->will_message);
      }
      if (mm->connect_flags & MG_MQTT_HAS_USER_NAME) {
        if (p >= end) return MG_MQTT_ERROR_MALFORMED_MSG;
        p = scanto(p, &mm->user_name);
      }
      if (mm->connect_flags & MG_MQTT_HAS_PASSWORD) {
        if (p >= end) return MG_MQTT_ERROR_MALFORMED_MSG;
        p = scanto(p, &mm->password);
      }
      if (p != end) return MG_MQTT_ERROR_MALFORMED_MSG;

      LOG(LL_DEBUG,
          ("%d %2x %d proto [%.*s] client_id [%.*s] will_topic [%.*s] "
           "will_msg [%.*s] user_name [%.*s] password [%.*s]",
           (int) len, (int) mm->connect_flags, (int) mm->keep_alive_timer,
           (int) mm->protocol_name.len, mm->protocol_name.p,
           (int) mm->client_id.len, mm->client_id.p, (int) mm->will_topic.len,
           mm->will_topic.p, (int) mm->will_message.len, mm->will_message.p,
           (int) mm->user_name.len, mm->user_name.p, (int) mm->password.len,
           mm->password.p));
      break;
    }
    case MG_MQTT_CMD_CONNACK:
      if (end - p < 2) return MG_MQTT_ERROR_MALFORMED_MSG;
      mm->connack_ret_code = p[1];
      break;
    case MG_MQTT_CMD_PUBACK:
    case MG_MQTT_CMD_PUBREC:
    case MG_MQTT_CMD_PUBREL:
    case MG_MQTT_CMD_PUBCOMP:
    case MG_MQTT_CMD_SUBACK:
      mm->message_id = getu16(p);
      break;
    case MG_MQTT_CMD_PUBLISH: {
      p = scanto(p, &mm->topic);
      if (p > end) return MG_MQTT_ERROR_MALFORMED_MSG;
      if (mm->qos > 0) {
        if (end - p < 2) return MG_MQTT_ERROR_MALFORMED_MSG;
        mm->message_id = getu16(p);
        p += 2;
      }
      mm->payload.p = p;
      mm->payload.len = end - p;
      break;
    }
    case MG_MQTT_CMD_SUBSCRIBE:
      if (end - p < 2) return MG_MQTT_ERROR_MALFORMED_MSG;
      mm->message_id = getu16(p);
      p += 2;
      /*
       * topic expressions are left in the payload and can be parsed with
       * `mg_mqtt_next_subscribe_topic`
       */
      mm->payload.p = p;
      mm->payload.len = end - p;
      break;
    default:
      /* Unhandled command */
      break;
  }

  mm->len = end - io->buf;
  return mm->len;
}