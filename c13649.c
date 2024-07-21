babel_packet_examin(const unsigned char *packet, int packetlen)
{
    unsigned i = 0, bodylen;
    const unsigned char *message;
    unsigned char type, len;

    if(packetlen < 4 || packet[0] != 42 || packet[1] != 2)
        return 1;
    DO_NTOHS(bodylen, packet + 2);
    while (i < bodylen){
        message = packet + 4 + i;
        type = message[0];
        if(type == MESSAGE_PAD1) {
            i++;
            continue;
        }
        if(i + 1 > bodylen) {
            debugf(BABEL_DEBUG_COMMON,"Received truncated message.");
            return 1;
        }
        len = message[1];
        if(i + len > bodylen) {
            debugf(BABEL_DEBUG_COMMON,"Received truncated message.");
            return 1;
        }
        /* not Pad1 */
        if(type <= MESSAGE_MAX && tlv_min_length[type] && len < tlv_min_length[type]) {
            debugf(BABEL_DEBUG_COMMON,"Undersized %u TLV", type);
            return 1;
        }
        i += len + 2;
    }
    return 0;
}