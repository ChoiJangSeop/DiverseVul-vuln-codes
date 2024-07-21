int usbredirparser_serialize(struct usbredirparser *parser_pub,
                             uint8_t **state_dest, int *state_len)
{
    struct usbredirparser_priv *parser =
        (struct usbredirparser_priv *)parser_pub;
    struct usbredirparser_buf *wbuf;
    uint8_t *write_buf_count_pos, *state = NULL, *pos = NULL;
    uint32_t write_buf_count = 0, len, remain = 0;

    *state_dest = NULL;
    *state_len = 0;

    if (serialize_int(parser, &state, &pos, &remain,
                                   USBREDIRPARSER_SERIALIZE_MAGIC, "magic"))
        return -1;

    /* To be replaced with length later */
    if (serialize_int(parser, &state, &pos, &remain, 0, "length"))
        return -1;

    if (serialize_data(parser, &state, &pos, &remain,
                       (uint8_t *)parser->our_caps,
                       USB_REDIR_CAPS_SIZE * sizeof(int32_t), "our_caps"))
        return -1;

    if (parser->have_peer_caps) {
        if (serialize_data(parser, &state, &pos, &remain,
                           (uint8_t *)parser->peer_caps,
                           USB_REDIR_CAPS_SIZE * sizeof(int32_t), "peer_caps"))
            return -1;
    } else {
        if (serialize_int(parser, &state, &pos, &remain, 0, "peer_caps_len"))
            return -1;
    }

    if (serialize_int(parser, &state, &pos, &remain, parser->to_skip, "skip"))
        return -1;

    if (serialize_data(parser, &state, &pos, &remain,
                       (uint8_t *)&parser->header, parser->header_read,
                       "header"))
        return -1;

    if (serialize_data(parser, &state, &pos, &remain,
                       parser->type_header, parser->type_header_read,
                       "type_header"))
        return -1;

    if (serialize_data(parser, &state, &pos, &remain,
                       parser->data, parser->data_read, "packet-data"))
        return -1;

    write_buf_count_pos = pos;
    /* To be replaced with write_buf_count later */
    if (serialize_int(parser, &state, &pos, &remain, 0, "write_buf_count"))
        return -1;

    wbuf = parser->write_buf;
    while (wbuf) {
        if (serialize_data(parser, &state, &pos, &remain,
                           wbuf->buf + wbuf->pos, wbuf->len - wbuf->pos,
                           "write-buf"))
            return -1;
        write_buf_count++;
        wbuf = wbuf->next;
    }
    /* Patch in write_buf_count */
    memcpy(write_buf_count_pos, &write_buf_count, sizeof(int32_t));

    /* Patch in length */
    len = pos - state;
    memcpy(state + sizeof(int32_t), &len, sizeof(int32_t));

    *state_dest = state;
    *state_len = len;

    return 0;
}