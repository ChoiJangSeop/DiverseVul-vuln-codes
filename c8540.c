int tcp_emu(struct socket *so, struct mbuf *m)
{
    Slirp *slirp = so->slirp;
    unsigned n1, n2, n3, n4, n5, n6;
    char buff[257];
    uint32_t laddr;
    unsigned lport;
    char *bptr;

    DEBUG_CALL("tcp_emu");
    DEBUG_ARG("so = %p", so);
    DEBUG_ARG("m = %p", m);

    switch (so->so_emu) {
        int x, i;

        /* TODO: IPv6 */
    case EMU_IDENT:
        /*
         * Identification protocol as per rfc-1413
         */

        {
            struct socket *tmpso;
            struct sockaddr_in addr;
            socklen_t addrlen = sizeof(struct sockaddr_in);
            char *eol = g_strstr_len(m->m_data, m->m_len, "\r\n");

            if (!eol) {
                return 1;
            }

            *eol = '\0';
            if (sscanf(m->m_data, "%u%*[ ,]%u", &n1, &n2) == 2) {
                HTONS(n1);
                HTONS(n2);
                /* n2 is the one on our host */
                for (tmpso = slirp->tcb.so_next; tmpso != &slirp->tcb;
                     tmpso = tmpso->so_next) {
                    if (tmpso->so_laddr.s_addr == so->so_laddr.s_addr &&
                        tmpso->so_lport == n2 &&
                        tmpso->so_faddr.s_addr == so->so_faddr.s_addr &&
                        tmpso->so_fport == n1) {
                        if (getsockname(tmpso->s, (struct sockaddr *)&addr,
                                        &addrlen) == 0)
                            n2 = addr.sin_port;
                        break;
                    }
                }
                NTOHS(n1);
                NTOHS(n2);
                m_inc(m, snprintf(NULL, 0, "%d,%d\r\n", n1, n2) + 1);
                m->m_len = snprintf(m->m_data, M_ROOM(m), "%d,%d\r\n", n1, n2);
                assert(m->m_len < M_ROOM(m));
            } else {
                *eol = '\r';
            }

            return 1;
        }

    case EMU_FTP: /* ftp */
        m_inc(m, m->m_len + 1);
        *(m->m_data + m->m_len) = 0; /* NUL terminate for strstr */
        if ((bptr = (char *)strstr(m->m_data, "ORT")) != NULL) {
            /*
             * Need to emulate the PORT command
             */
            x = sscanf(bptr, "ORT %u,%u,%u,%u,%u,%u\r\n%256[^\177]", &n1, &n2,
                       &n3, &n4, &n5, &n6, buff);
            if (x < 6)
                return 1;

            laddr = htonl((n1 << 24) | (n2 << 16) | (n3 << 8) | (n4));
            lport = htons((n5 << 8) | (n6));

            if ((so = tcp_listen(slirp, INADDR_ANY, 0, laddr, lport,
                                 SS_FACCEPTONCE)) == NULL) {
                return 1;
            }
            n6 = ntohs(so->so_fport);

            n5 = (n6 >> 8) & 0xff;
            n6 &= 0xff;

            laddr = ntohl(so->so_faddr.s_addr);

            n1 = ((laddr >> 24) & 0xff);
            n2 = ((laddr >> 16) & 0xff);
            n3 = ((laddr >> 8) & 0xff);
            n4 = (laddr & 0xff);

            m->m_len = bptr - m->m_data; /* Adjust length */
            m->m_len += snprintf(bptr, m->m_size - m->m_len,
                                 "ORT %d,%d,%d,%d,%d,%d\r\n%s", n1, n2, n3, n4,
                                 n5, n6, x == 7 ? buff : "");
            return 1;
        } else if ((bptr = (char *)strstr(m->m_data, "27 Entering")) != NULL) {
            /*
             * Need to emulate the PASV response
             */
            x = sscanf(
                bptr,
                "27 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\r\n%256[^\177]",
                &n1, &n2, &n3, &n4, &n5, &n6, buff);
            if (x < 6)
                return 1;

            laddr = htonl((n1 << 24) | (n2 << 16) | (n3 << 8) | (n4));
            lport = htons((n5 << 8) | (n6));

            if ((so = tcp_listen(slirp, INADDR_ANY, 0, laddr, lport,
                                 SS_FACCEPTONCE)) == NULL) {
                return 1;
            }
            n6 = ntohs(so->so_fport);

            n5 = (n6 >> 8) & 0xff;
            n6 &= 0xff;

            laddr = ntohl(so->so_faddr.s_addr);

            n1 = ((laddr >> 24) & 0xff);
            n2 = ((laddr >> 16) & 0xff);
            n3 = ((laddr >> 8) & 0xff);
            n4 = (laddr & 0xff);

            m->m_len = bptr - m->m_data; /* Adjust length */
            m->m_len +=
                snprintf(bptr, m->m_size - m->m_len,
                         "27 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n%s",
                         n1, n2, n3, n4, n5, n6, x == 7 ? buff : "");

            return 1;
        }

        return 1;

    case EMU_KSH:
        /*
         * The kshell (Kerberos rsh) and shell services both pass
         * a local port port number to carry signals to the server
         * and stderr to the client.  It is passed at the beginning
         * of the connection as a NUL-terminated decimal ASCII string.
         */
        so->so_emu = 0;
        for (lport = 0, i = 0; i < m->m_len - 1; ++i) {
            if (m->m_data[i] < '0' || m->m_data[i] > '9')
                return 1; /* invalid number */
            lport *= 10;
            lport += m->m_data[i] - '0';
        }
        if (m->m_data[m->m_len - 1] == '\0' && lport != 0 &&
            (so = tcp_listen(slirp, INADDR_ANY, 0, so->so_laddr.s_addr,
                             htons(lport), SS_FACCEPTONCE)) != NULL)
            m->m_len =
                snprintf(m->m_data, m->m_size, "%d", ntohs(so->so_fport)) + 1;
        return 1;

    case EMU_IRC:
        /*
         * Need to emulate DCC CHAT, DCC SEND and DCC MOVE
         */
        m_inc(m, m->m_len + 1);
        *(m->m_data + m->m_len) = 0; /* NULL terminate the string for strstr */
        if ((bptr = (char *)strstr(m->m_data, "DCC")) == NULL)
            return 1;

        /* The %256s is for the broken mIRC */
        if (sscanf(bptr, "DCC CHAT %256s %u %u", buff, &laddr, &lport) == 3) {
            if ((so = tcp_listen(slirp, INADDR_ANY, 0, htonl(laddr),
                                 htons(lport), SS_FACCEPTONCE)) == NULL) {
                return 1;
            }
            m->m_len = bptr - m->m_data; /* Adjust length */
            m->m_len += snprintf(bptr, m->m_size, "DCC CHAT chat %lu %u%c\n",
                                 (unsigned long)ntohl(so->so_faddr.s_addr),
                                 ntohs(so->so_fport), 1);
        } else if (sscanf(bptr, "DCC SEND %256s %u %u %u", buff, &laddr, &lport,
                          &n1) == 4) {
            if ((so = tcp_listen(slirp, INADDR_ANY, 0, htonl(laddr),
                                 htons(lport), SS_FACCEPTONCE)) == NULL) {
                return 1;
            }
            m->m_len = bptr - m->m_data; /* Adjust length */
            m->m_len +=
                snprintf(bptr, m->m_size, "DCC SEND %s %lu %u %u%c\n", buff,
                         (unsigned long)ntohl(so->so_faddr.s_addr),
                         ntohs(so->so_fport), n1, 1);
        } else if (sscanf(bptr, "DCC MOVE %256s %u %u %u", buff, &laddr, &lport,
                          &n1) == 4) {
            if ((so = tcp_listen(slirp, INADDR_ANY, 0, htonl(laddr),
                                 htons(lport), SS_FACCEPTONCE)) == NULL) {
                return 1;
            }
            m->m_len = bptr - m->m_data; /* Adjust length */
            m->m_len +=
                snprintf(bptr, m->m_size, "DCC MOVE %s %lu %u %u%c\n", buff,
                         (unsigned long)ntohl(so->so_faddr.s_addr),
                         ntohs(so->so_fport), n1, 1);
        }
        return 1;

    case EMU_REALAUDIO:
        /*
         * RealAudio emulation - JP. We must try to parse the incoming
         * data and try to find the two characters that contain the
         * port number. Then we redirect an udp port and replace the
         * number with the real port we got.
         *
         * The 1.0 beta versions of the player are not supported
         * any more.
         *
         * A typical packet for player version 1.0 (release version):
         *
         * 0000:50 4E 41 00 05
         * 0000:00 01 00 02 1B D7 00 00 67 E6 6C DC 63 00 12 50 ........g.l.c..P
         * 0010:4E 43 4C 49 45 4E 54 20 31 30 31 20 41 4C 50 48 NCLIENT 101 ALPH
         * 0020:41 6C 00 00 52 00 17 72 61 66 69 6C 65 73 2F 76 Al..R..rafiles/v
         * 0030:6F 61 2F 65 6E 67 6C 69 73 68 5F 2E 72 61 79 42 oa/english_.rayB
         *
         * Now the port number 0x1BD7 is found at offset 0x04 of the
         * Now the port number 0x1BD7 is found at offset 0x04 of the
         * second packet. This time we received five bytes first and
         * then the rest. You never know how many bytes you get.
         *
         * A typical packet for player version 2.0 (beta):
         *
         * 0000:50 4E 41 00 06 00 02 00 00 00 01 00 02 1B C1 00 PNA.............
         * 0010:00 67 75 78 F5 63 00 0A 57 69 6E 32 2E 30 2E 30 .gux.c..Win2.0.0
         * 0020:2E 35 6C 00 00 52 00 1C 72 61 66 69 6C 65 73 2F .5l..R..rafiles/
         * 0030:77 65 62 73 69 74 65 2F 32 30 72 65 6C 65 61 73 website/20releas
         * 0040:65 2E 72 61 79 53 00 00 06 36 42                e.rayS...6B
         *
         * Port number 0x1BC1 is found at offset 0x0d.
         *
         * This is just a horrible switch statement. Variable ra tells
         * us where we're going.
         */

        bptr = m->m_data;
        while (bptr < m->m_data + m->m_len) {
            uint16_t p;
            static int ra = 0;
            char ra_tbl[4];

            ra_tbl[0] = 0x50;
            ra_tbl[1] = 0x4e;
            ra_tbl[2] = 0x41;
            ra_tbl[3] = 0;

            switch (ra) {
            case 0:
            case 2:
            case 3:
                if (*bptr++ != ra_tbl[ra]) {
                    ra = 0;
                    continue;
                }
                break;

            case 1:
                /*
                 * We may get 0x50 several times, ignore them
                 */
                if (*bptr == 0x50) {
                    ra = 1;
                    bptr++;
                    continue;
                } else if (*bptr++ != ra_tbl[ra]) {
                    ra = 0;
                    continue;
                }
                break;

            case 4:
                /*
                 * skip version number
                 */
                bptr++;
                break;

            case 5:
                /*
                 * The difference between versions 1.0 and
                 * 2.0 is here. For future versions of
                 * the player this may need to be modified.
                 */
                if (*(bptr + 1) == 0x02)
                    bptr += 8;
                else
                    bptr += 4;
                break;

            case 6:
                /* This is the field containing the port
                 * number that RA-player is listening to.
                 */
                lport = (((uint8_t *)bptr)[0] << 8) + ((uint8_t *)bptr)[1];
                if (lport < 6970)
                    lport += 256; /* don't know why */
                if (lport < 6970 || lport > 7170)
                    return 1; /* failed */

                /* try to get udp port between 6970 - 7170 */
                for (p = 6970; p < 7071; p++) {
                    if (udp_listen(slirp, INADDR_ANY, htons(p),
                                   so->so_laddr.s_addr, htons(lport),
                                   SS_FACCEPTONCE)) {
                        break;
                    }
                }
                if (p == 7071)
                    p = 0;
                *(uint8_t *)bptr++ = (p >> 8) & 0xff;
                *(uint8_t *)bptr = p & 0xff;
                ra = 0;
                return 1; /* port redirected, we're done */
                break;

            default:
                ra = 0;
            }
            ra++;
        }
        return 1;

    default:
        /* Ooops, not emulated, won't call tcp_emu again */
        so->so_emu = 0;
        return 1;
    }
}