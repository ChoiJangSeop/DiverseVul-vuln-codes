dns_transmit_io (struct dns_transmit *d, iopause_fd *x, struct taia *deadline)
{
    x->fd = d->s1 - 1;

    switch (d->tcpstate)
    {
    case 0:
    case 3:
    case 4:
    case 5:
        x->events = IOPAUSE_READ;
        break;

    case 1:
    case 2:
        x->events = IOPAUSE_WRITE;
    }

    if (taia_less (&d->deadline, deadline))
        *deadline = d->deadline;
}