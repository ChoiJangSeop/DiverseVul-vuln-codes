addr_policy_get_canonical_entry(addr_policy_t *e)
{
  policy_map_ent_t search, *found;
  if (e->is_canonical)
    return e;

  search.policy = e;
  found = HT_FIND(policy_map, &policy_root, &search);
  if (!found) {
    found = tor_malloc_zero(sizeof(policy_map_ent_t));
    found->policy = tor_memdup(e, sizeof(addr_policy_t));
    found->policy->is_canonical = 1;
    found->policy->refcnt = 0;
    HT_INSERT(policy_map, &policy_root, found);
  }

  tor_assert(!cmp_single_addr_policy(found->policy, e));
  ++found->policy->refcnt;
  return found->policy;
}