delegpt_add_rrset_A(struct delegpt* dp, struct regional* region,
	struct ub_packed_rrset_key* ak, uint8_t lame)
{
        struct packed_rrset_data* d=(struct packed_rrset_data*)ak->entry.data;
        size_t i;
        struct sockaddr_in sa;
        socklen_t len = (socklen_t)sizeof(sa);
	log_assert(!dp->dp_type_mlc);
        memset(&sa, 0, len);
        sa.sin_family = AF_INET;
        sa.sin_port = (in_port_t)htons(UNBOUND_DNS_PORT);
        for(i=0; i<d->count; i++) {
                if(d->rr_len[i] != 2 + INET_SIZE)
                        continue;
                memmove(&sa.sin_addr, d->rr_data[i]+2, INET_SIZE);
                if(!delegpt_add_target(dp, region, ak->rk.dname,
                        ak->rk.dname_len, (struct sockaddr_storage*)&sa,
                        len, (d->security==sec_status_bogus), lame))
                        return 0;
        }
        return 1;
}