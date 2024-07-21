cmp_single_addr_policy(addr_policy_t *a, addr_policy_t *b)
{
  int r;
  if ((r=((int)a->policy_type - (int)b->policy_type)))
    return r;
  if ((r=((int)a->is_private - (int)b->is_private)))
    return r;
  /* refcnt and is_canonical are irrelevant to equality,
   * they are hash table implementation details */
  if ((r=tor_addr_compare(&a->addr, &b->addr, CMP_EXACT)))
    return r;
  if ((r=((int)a->maskbits - (int)b->maskbits)))
    return r;
  if ((r=((int)a->prt_min - (int)b->prt_min)))
    return r;
  if ((r=((int)a->prt_max - (int)b->prt_max)))
    return r;
  return 0;
}