dns_transmit_get (struct dns_transmit *d, const iopause_fd *x,
                                          const struct taia *when)
{
    char udpbuf[4097];
    int r = 0, fd = 0;
    unsigned char ch = 0;

    fd = d->s1 - 1;
    errno = error_io;

    if (!x->revents)
    {
        if (taia_less (when, &d->deadline))
            return 0;
        errno = error_timeout;
        if (d->tcpstate == 0)
            return nextudp (d);

        return nexttcp (d);
    }

    if (d->tcpstate == 0)
    {
        /*
         * have attempted to send UDP query to each server udploop times
         * have sent query to curserver on UDP socket s
         */
        r = recv (fd, udpbuf, sizeof (udpbuf), 0);
        if (r <= 0)
        {
            if (errno == error_connrefused && d->udploop == 2)
                    return 0;

            return nextudp (d);
        }
        if (r + 1 > sizeof (udpbuf))
            return 0;

        if (irrelevant (d, udpbuf, r))
            return 0;
        if (serverwantstcp (udpbuf, r))
            return firsttcp (d);
        if (serverfailed (udpbuf, r))
        {
            if (d->udploop == 2)
                return 0;

            return nextudp (d);
        }
        socketfree (d);

        d->packetlen = r;
        d->packet = alloc (d->packetlen);
        if (!d->packet)
        {
            dns_transmit_free (d);
            return -1;
        }
        byte_copy (d->packet, d->packetlen, udpbuf);

        queryfree (d);
        return 1;
    }

    if (d->tcpstate == 1)
    {
        /*
         * have sent connection attempt to curserver on TCP socket s
         * pos not defined
         */
        if (!socket_connected (fd))
            return nexttcp (d);

        d->pos = 0;
        d->tcpstate = 2;

        return 0;
    }

    if (d->tcpstate == 2)
    {
        /* 
         * have connection to curserver on TCP socket s have sent pos bytes
         * of query
         */
        r = write (fd, d->query + d->pos, d->querylen - d->pos);
        if (r <= 0)
            return nexttcp (d);

        d->pos += r;
        if (d->pos == d->querylen)
        {
            struct taia now;

            taia_now (&now);
            taia_uint (&d->deadline, 10);
            taia_add (&d->deadline, &d->deadline, &now);
            d->tcpstate = 3;
        }

        return 0;
    }

    if (d->tcpstate == 3)
    {
        /*
         * have sent entire query to curserver on TCP socket s
         * pos not defined
         */
        r = read (fd, &ch, 1);
        if (r <= 0)
            return nexttcp (d);
        
        d->packetlen = ch;
        d->tcpstate = 4;

        return 0;
    }

    if (d->tcpstate == 4)
    {
        /*
         * have sent entire query to curserver on TCP socket s
         * pos not defined
         * have received one byte of packet length into packetlen
         */
        r = read (fd, &ch, 1);
        if (r <= 0)
            return nexttcp (d);

        d->pos = 0;
        d->tcpstate = 5;
        d->packetlen <<= 8;
        d->packetlen += ch;
        d->packet = alloc (d->packetlen);
        if (!d->packet)
        {
            dns_transmit_free (d);
            return -1;
        }

        return 0;
    }

    if (d->tcpstate == 5)
    {
        /*
         * have sent entire query to curserver on TCP socket s have received
         * entire packet length into packetlen packet is allocated have
         * received pos bytes of packet
         */
        r = read (fd, d->packet + d->pos, d->packetlen - d->pos);
        if (r <= 0)
            return nexttcp (d);

        d->pos += r;
        if (d->pos < d->packetlen)
            return 0;

        socketfree (d);
        if (irrelevant (d, d->packet, d->packetlen))
            return nexttcp(d);
        if (serverwantstcp (d->packet, d->packetlen))
            return nexttcp(d);
        if (serverfailed (d->packet, d->packetlen))
            return nexttcp(d);

        queryfree(d);
        return 1;
    }

    return 0;
}