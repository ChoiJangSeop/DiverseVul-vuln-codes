thistcp (struct dns_transmit *d)
{
    struct taia now;
    const char *ip = NULL;

    socketfree (d);
    packetfree (d);

    for (; d->curserver < 16; ++d->curserver)
    {
        ip = d->servers + 4 * d->curserver;
        if (byte_diff (ip, 4, "\0\0\0\0"))
        {
            d->query[2] = dns_random (256);
            d->query[3] = dns_random (256);

            d->s1 = 1 + socket_tcp ();
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
  
            taia_now (&now);
            taia_uint (&d->deadline, 10);
            taia_add (&d->deadline, &d->deadline, &now);
            if (socket_connect4 (d->s1 - 1, ip, 53) == 0)
            {
                d->pos = 0;
                d->tcpstate = 2;
                return 0;
            }
            if (errno == error_inprogress || errno == error_wouldblock)
            {
                d->tcpstate = 1;
                return 0;
            }
            socketfree(d);
        }
    }

    dns_transmit_free(d);
    return -1;
}