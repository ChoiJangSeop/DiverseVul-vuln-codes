query_io (struct query *z, iopause_fd *x, struct taia *deadline)
{
    dns_transmit_io (&z->dt, x, deadline);
}