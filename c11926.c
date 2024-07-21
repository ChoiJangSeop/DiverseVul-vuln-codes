query_get (struct query *z, iopause_fd *x, struct taia *stamp)
{
    switch (dns_transmit_get (&z->dt, x, stamp))
    {
    case 1:
        return doit (z, 1);
    case -1:
        return doit (z, -1);
    }

    return 0;
}