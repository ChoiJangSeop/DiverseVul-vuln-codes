int
router_differences_are_cosmetic(const routerinfo_t *r1, const routerinfo_t *r2)
{
  time_t r1pub, r2pub;
  long time_difference;
  tor_assert(r1 && r2);

  /* r1 should be the one that was published first. */
  if (r1->cache_info.published_on > r2->cache_info.published_on) {
    const routerinfo_t *ri_tmp = r2;
    r2 = r1;
    r1 = ri_tmp;
  }

  /* If any key fields differ, they're different. */
  if (r1->addr != r2->addr ||
      strcasecmp(r1->nickname, r2->nickname) ||
      r1->or_port != r2->or_port ||
      !tor_addr_eq(&r1->ipv6_addr, &r2->ipv6_addr) ||
      r1->ipv6_orport != r2->ipv6_orport ||
      r1->dir_port != r2->dir_port ||
      r1->purpose != r2->purpose ||
      !crypto_pk_eq_keys(r1->onion_pkey, r2->onion_pkey) ||
      !crypto_pk_eq_keys(r1->identity_pkey, r2->identity_pkey) ||
      strcasecmp(r1->platform, r2->platform) ||
      (r1->contact_info && !r2->contact_info) || /* contact_info is optional */
      (!r1->contact_info && r2->contact_info) ||
      (r1->contact_info && r2->contact_info &&
       strcasecmp(r1->contact_info, r2->contact_info)) ||
      r1->is_hibernating != r2->is_hibernating ||
      cmp_addr_policies(r1->exit_policy, r2->exit_policy) ||
      (r1->supports_tunnelled_dir_requests !=
       r2->supports_tunnelled_dir_requests))
    return 0;
  if ((r1->declared_family == NULL) != (r2->declared_family == NULL))
    return 0;
  if (r1->declared_family && r2->declared_family) {
    int i, n;
    if (smartlist_len(r1->declared_family)!=smartlist_len(r2->declared_family))
      return 0;
    n = smartlist_len(r1->declared_family);
    for (i=0; i < n; ++i) {
      if (strcasecmp(smartlist_get(r1->declared_family, i),
                     smartlist_get(r2->declared_family, i)))
        return 0;
    }
  }

  /* Did bandwidth change a lot? */
  if ((r1->bandwidthcapacity < r2->bandwidthcapacity/2) ||
      (r2->bandwidthcapacity < r1->bandwidthcapacity/2))
    return 0;

  /* Did the bandwidthrate or bandwidthburst change? */
  if ((r1->bandwidthrate != r2->bandwidthrate) ||
      (r1->bandwidthburst != r2->bandwidthburst))
    return 0;

  /* Did more than 12 hours pass? */
  if (r1->cache_info.published_on + ROUTER_MAX_COSMETIC_TIME_DIFFERENCE
      < r2->cache_info.published_on)
    return 0;

  /* Did uptime fail to increase by approximately the amount we would think,
   * give or take some slop? */
  r1pub = r1->cache_info.published_on;
  r2pub = r2->cache_info.published_on;
  time_difference = labs(r2->uptime - (r1->uptime + (r2pub - r1pub)));
  if (time_difference > ROUTER_ALLOW_UPTIME_DRIFT &&
      time_difference > r1->uptime * .05 &&
      time_difference > r2->uptime * .05)
    return 0;

  /* Otherwise, the difference is cosmetic. */
  return 1;