smartlist_choose_by_bandwidth(smartlist_t *sl, bandwidth_weight_rule_t rule,
                              int statuses)
{
  unsigned int i;
  routerinfo_t *router;
  routerstatus_t *status=NULL;
  int32_t *bandwidths;
  int is_exit;
  int is_guard;
  uint64_t total_nonexit_bw = 0, total_exit_bw = 0, total_bw = 0;
  uint64_t total_nonguard_bw = 0, total_guard_bw = 0;
  uint64_t rand_bw, tmp;
  double exit_weight;
  double guard_weight;
  int n_unknown = 0;
  bitarray_t *exit_bits;
  bitarray_t *guard_bits;
  int me_idx = -1;

  // This function does not support WEIGHT_FOR_DIR
  // or WEIGHT_FOR_MID
  if (rule == WEIGHT_FOR_DIR || rule == WEIGHT_FOR_MID) {
    rule = NO_WEIGHTING;
  }

  /* Can't choose exit and guard at same time */
  tor_assert(rule == NO_WEIGHTING ||
             rule == WEIGHT_FOR_EXIT ||
             rule == WEIGHT_FOR_GUARD);

  if (smartlist_len(sl) == 0) {
    log_info(LD_CIRC,
             "Empty routerlist passed in to old node selection for rule %s",
             bandwidth_weight_rule_to_string(rule));
    return NULL;
  }

  /* First count the total bandwidth weight, and make a list
   * of each value.  <0 means "unknown; no routerinfo."  We use the
   * bits of negative values to remember whether the router was fast (-x)&1
   * and whether it was an exit (-x)&2 or guard (-x)&4.  Yes, it's a hack. */
  bandwidths = tor_malloc(sizeof(int32_t)*smartlist_len(sl));
  exit_bits = bitarray_init_zero(smartlist_len(sl));
  guard_bits = bitarray_init_zero(smartlist_len(sl));

  /* Iterate over all the routerinfo_t or routerstatus_t, and */
  for (i = 0; i < (unsigned)smartlist_len(sl); ++i) {
    /* first, learn what bandwidth we think i has */
    int is_known = 1;
    int32_t flags = 0;
    uint32_t this_bw = 0;
    if (statuses) {
      status = smartlist_get(sl, i);
      if (router_digest_is_me(status->identity_digest))
        me_idx = i;
      router = router_get_by_digest(status->identity_digest);
      is_exit = status->is_exit;
      is_guard = status->is_possible_guard;
      if (status->has_bandwidth) {
        this_bw = kb_to_bytes(status->bandwidth);
      } else { /* guess */
        /* XXX023 once consensuses always list bandwidths, we can take
         * this guessing business out. -RD */
        is_known = 0;
        flags = status->is_fast ? 1 : 0;
        flags |= is_exit ? 2 : 0;
        flags |= is_guard ? 4 : 0;
      }
    } else {
      routerstatus_t *rs;
      router = smartlist_get(sl, i);
      rs = router_get_consensus_status_by_id(
             router->cache_info.identity_digest);
      if (router_digest_is_me(router->cache_info.identity_digest))
        me_idx = i;
      is_exit = router->is_exit;
      is_guard = router->is_possible_guard;
      if (rs && rs->has_bandwidth) {
        this_bw = kb_to_bytes(rs->bandwidth);
      } else if (rs) { /* guess; don't trust the descriptor */
        /* XXX023 once consensuses always list bandwidths, we can take
         * this guessing business out. -RD */
        is_known = 0;
        flags = router->is_fast ? 1 : 0;
        flags |= is_exit ? 2 : 0;
        flags |= is_guard ? 4 : 0;
      } else /* bridge or other descriptor not in our consensus */
        this_bw = bridge_get_advertised_bandwidth_bounded(router);
    }
    if (is_exit)
      bitarray_set(exit_bits, i);
    if (is_guard)
      bitarray_set(guard_bits, i);
    if (is_known) {
      bandwidths[i] = (int32_t) this_bw; // safe since MAX_BELIEVABLE<INT32_MAX
      // XXX this is no longer true! We don't always cap the bw anymore. Can
      // a consensus make us overflow?-sh
      tor_assert(bandwidths[i] >= 0);
      if (is_guard)
        total_guard_bw += this_bw;
      else
        total_nonguard_bw += this_bw;
      if (is_exit)
        total_exit_bw += this_bw;
      else
        total_nonexit_bw += this_bw;
    } else {
      ++n_unknown;
      bandwidths[i] = -flags;
    }
  }

  /* Now, fill in the unknown values. */
  if (n_unknown) {
    int32_t avg_fast, avg_slow;
    if (total_exit_bw+total_nonexit_bw) {
      /* if there's some bandwidth, there's at least one known router,
       * so no worries about div by 0 here */
      int n_known = smartlist_len(sl)-n_unknown;
      avg_fast = avg_slow = (int32_t)
        ((total_exit_bw+total_nonexit_bw)/((uint64_t) n_known));
    } else {
      avg_fast = 40000;
      avg_slow = 20000;
    }
    for (i=0; i<(unsigned)smartlist_len(sl); ++i) {
      int32_t bw = bandwidths[i];
      if (bw>=0)
        continue;
      is_exit = ((-bw)&2);
      is_guard = ((-bw)&4);
      bandwidths[i] = ((-bw)&1) ? avg_fast : avg_slow;
      if (is_exit)
        total_exit_bw += bandwidths[i];
      else
        total_nonexit_bw += bandwidths[i];
      if (is_guard)
        total_guard_bw += bandwidths[i];
      else
        total_nonguard_bw += bandwidths[i];
    }
  }

  /* If there's no bandwidth at all, pick at random. */
  if (!(total_exit_bw+total_nonexit_bw)) {
    tor_free(bandwidths);
    tor_free(exit_bits);
    tor_free(guard_bits);
    return smartlist_choose(sl);
  }

  /* Figure out how to weight exits and guards */
  {
    double all_bw = U64_TO_DBL(total_exit_bw+total_nonexit_bw);
    double exit_bw = U64_TO_DBL(total_exit_bw);
    double guard_bw = U64_TO_DBL(total_guard_bw);
    /*
     * For detailed derivation of this formula, see
     *   http://archives.seul.org/or/dev/Jul-2007/msg00056.html
     */
    if (rule == WEIGHT_FOR_EXIT || !total_exit_bw)
      exit_weight = 1.0;
    else
      exit_weight = 1.0 - all_bw/(3.0*exit_bw);

    if (rule == WEIGHT_FOR_GUARD || !total_guard_bw)
      guard_weight = 1.0;
    else
      guard_weight = 1.0 - all_bw/(3.0*guard_bw);

    if (exit_weight <= 0.0)
      exit_weight = 0.0;

    if (guard_weight <= 0.0)
      guard_weight = 0.0;

    total_bw = 0;
    sl_last_weighted_bw_of_me = 0;
    for (i=0; i < (unsigned)smartlist_len(sl); i++) {
      uint64_t bw;
      is_exit = bitarray_is_set(exit_bits, i);
      is_guard = bitarray_is_set(guard_bits, i);
      if (is_exit && is_guard)
        bw = ((uint64_t)(bandwidths[i] * exit_weight * guard_weight));
      else if (is_guard)
        bw = ((uint64_t)(bandwidths[i] * guard_weight));
      else if (is_exit)
        bw = ((uint64_t)(bandwidths[i] * exit_weight));
      else
        bw = bandwidths[i];
      total_bw += bw;
      if (i == (unsigned) me_idx)
        sl_last_weighted_bw_of_me = bw;
    }
  }

  /* XXXX023 this is a kludge to expose these values. */
  sl_last_total_weighted_bw = total_bw;

  log_debug(LD_CIRC, "Total weighted bw = "U64_FORMAT
            ", exit bw = "U64_FORMAT
            ", nonexit bw = "U64_FORMAT", exit weight = %f "
            "(for exit == %d)"
            ", guard bw = "U64_FORMAT
            ", nonguard bw = "U64_FORMAT", guard weight = %f "
            "(for guard == %d)",
            U64_PRINTF_ARG(total_bw),
            U64_PRINTF_ARG(total_exit_bw), U64_PRINTF_ARG(total_nonexit_bw),
            exit_weight, (int)(rule == WEIGHT_FOR_EXIT),
            U64_PRINTF_ARG(total_guard_bw), U64_PRINTF_ARG(total_nonguard_bw),
            guard_weight, (int)(rule == WEIGHT_FOR_GUARD));

  /* Almost done: choose a random value from the bandwidth weights. */
  rand_bw = crypto_rand_uint64(total_bw);
  rand_bw++; /* crypto_rand_uint64() counts from 0, and we need to count
              * from 1 below. See bug 1203 for details. */

  /* Last, count through sl until we get to the element we picked */
  tmp = 0;
  for (i=0; i < (unsigned)smartlist_len(sl); i++) {
    is_exit = bitarray_is_set(exit_bits, i);
    is_guard = bitarray_is_set(guard_bits, i);

    /* Weights can be 0 if not counting guards/exits */
    if (is_exit && is_guard)
      tmp += ((uint64_t)(bandwidths[i] * exit_weight * guard_weight));
    else if (is_guard)
      tmp += ((uint64_t)(bandwidths[i] * guard_weight));
    else if (is_exit)
      tmp += ((uint64_t)(bandwidths[i] * exit_weight));
    else
      tmp += bandwidths[i];

    if (tmp >= rand_bw)
      break;
  }
  if (i == (unsigned)smartlist_len(sl)) {
    /* This was once possible due to round-off error, but shouldn't be able
     * to occur any longer. */
    tor_fragile_assert();
    --i;
    log_warn(LD_BUG, "Round-off error in computing bandwidth had an effect on "
             " which router we chose. Please tell the developers. "
             U64_FORMAT " " U64_FORMAT " " U64_FORMAT, U64_PRINTF_ARG(tmp),
             U64_PRINTF_ARG(rand_bw), U64_PRINTF_ARG(total_bw));
  }
  tor_free(bandwidths);
  tor_free(exit_bits);
  tor_free(guard_bits);
  return smartlist_get(sl, i);
}