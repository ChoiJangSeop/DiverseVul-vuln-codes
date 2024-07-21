thisudp (struct dns_transmit *d)
{
    const char *ip = NULL;

    socketfree (d);

    while (d->udploop < 4)
    {
        for (; d->curserver < 16; ++d->curserver)
        {
            ip = d->servers + 4 * d->curserver;
            if (byte_diff (ip, 4, "\0\0\0\0"))
            {
                d->query[2] = dns_random (256);
                d->query[3] = dns_random (256);
  
                d->s1 = 1 + socket_udp ();
                if (!d->s1)
                {
                    dns_transmit_free (d);
                    return -1;
                }
                if (randombind (d) == -1)
                {
                    dns_transmit_free (d);
                    return -1;
                }

                if (socket_connect4 (d->s1 - 1, ip, 53) == 0)
                {
                    if (send (d->s1 - 1, d->query + 2, d->querylen - 2, 0)
                            == d->querylen - 2)
                    {
                        struct taia now;

                        taia_now (&now);
                        taia_uint (&d->deadline, timeouts[d->udploop]);
                        taia_add (&d->deadline, &d->deadline, &now);
                        d->tcpstate = 0;

                        return 0;
                    }
                }
                socketfree (d);
            }
        }

        ++d->udploop;
        d->curserver = 0;
    }

    dns_transmit_free (d);
    return -1;
}