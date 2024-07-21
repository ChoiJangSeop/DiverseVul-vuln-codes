policy_eq(policy_map_ent_t *a, policy_map_ent_t *b)
{
  return cmp_single_addr_policy(a->policy, b->policy) == 0;
}