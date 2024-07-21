parse_update_subtlv(const unsigned char *a, int alen,
                    unsigned char *channels)
{
    int type, len, i = 0;

    while(i < alen) {
        type = a[i];
        if(type == SUBTLV_PAD1) {
            i++;
            continue;
        }

        if(i + 1 > alen) {
            flog_err(EC_BABEL_PACKET, "Received truncated attributes.");
            return;
        }
        len = a[i + 1];
        if(i + len > alen) {
            flog_err(EC_BABEL_PACKET, "Received truncated attributes.");
            return;
        }

        if(type == SUBTLV_PADN) {
            /* Nothing. */
        } else if(type == SUBTLV_DIVERSITY) {
            if(len > DIVERSITY_HOPS) {
                flog_err(EC_BABEL_PACKET,
			  "Received overlong channel information (%d > %d).n",
                          len, DIVERSITY_HOPS);
                len = DIVERSITY_HOPS;
            }
            if(memchr(a + i + 2, 0, len) != NULL) {
                /* 0 is reserved. */
                flog_err(EC_BABEL_PACKET, "Channel information contains 0!");
                return;
            }
            memset(channels, 0, DIVERSITY_HOPS);
            memcpy(channels, a + i + 2, len);
        } else {
            debugf(BABEL_DEBUG_COMMON,
                   "Received unknown route attribute %d.", type);
        }

        i += len + 2;
    }
}