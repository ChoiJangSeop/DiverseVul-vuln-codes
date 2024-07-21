ipv4_sanity_check(const struct ip_header *nh, size_t size,
                  int *ip_lenp, uint16_t *tot_lenp)
{
    int ip_len;
    uint16_t tot_len;

    if (OVS_UNLIKELY(size < IP_HEADER_LEN)) {
        return false;
    }
    ip_len = IP_IHL(nh->ip_ihl_ver) * 4;

    if (OVS_UNLIKELY(ip_len < IP_HEADER_LEN || size < ip_len)) {
        return false;
    }

    tot_len = ntohs(nh->ip_tot_len);
    if (OVS_UNLIKELY(tot_len > size || ip_len > tot_len ||
                size - tot_len > UINT8_MAX)) {
        return false;
    }

    *ip_lenp = ip_len;
    *tot_lenp = tot_len;

    return true;
}