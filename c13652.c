parse_hello_subtlv(const unsigned char *a, int alen,
                   unsigned int *hello_send_us)
{
    int type, len, i = 0, ret = 0;

    while(i < alen) {
        type = a[0];
        if(type == SUBTLV_PAD1) {
            i++;
            continue;
        }

        if(i + 1 > alen) {
            flog_err(EC_BABEL_PACKET,
		      "Received truncated sub-TLV on Hello message.");
            return -1;
        }
        len = a[i + 1];
        if(i + len > alen) {
            flog_err(EC_BABEL_PACKET,
		      "Received truncated sub-TLV on Hello message.");
            return -1;
        }

        if(type == SUBTLV_PADN) {
            /* Nothing to do. */
        } else if(type == SUBTLV_TIMESTAMP) {
            if(len >= 4) {
                DO_NTOHL(*hello_send_us, a + i + 2);
                ret = 1;
            } else {
                flog_err(EC_BABEL_PACKET,
			  "Received incorrect RTT sub-TLV on Hello message.");
            }
        } else {
            debugf(BABEL_DEBUG_COMMON,
                   "Received unknown Hello sub-TLV type %d.", type);
        }

        i += len + 2;
    }
    return ret;
}