queryfree (struct dns_transmit *d)
{
    if (!d->query)
      return;
    alloc_free (d->query);
    d->query = 0;
}