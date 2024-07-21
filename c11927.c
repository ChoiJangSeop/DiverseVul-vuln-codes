cleanup (struct query *z)
{
    int j = 0, k = 0;

    dns_transmit_free (&z->dt);
    for (j = 0; j < QUERY_MAXALIAS; ++j)
        dns_domain_free (&z->alias[j]);

    for (j = 0; j < QUERY_MAXLEVEL; ++j)
    {
        dns_domain_free (&z->name[j]);
        for (k = 0; k < QUERY_MAXNS; ++k)
            dns_domain_free (&z->ns[j][k]);
    }
}