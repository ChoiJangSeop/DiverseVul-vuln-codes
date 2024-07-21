smartlist_choose_by_bandwidth_weights(smartlist_t *sl,
                                      bandwidth_weight_rule_t rule,
                                      int statuses)
{
  int64_t weight_scale;
  int64_t rand_bw;
  double Wg = -1, Wm = -1, We = -1, Wd = -1;
  double Wgb = -1, Wmb = -1, Web = -1, Wdb = -1;
  double weighted_bw = 0;
  double *bandwidths;
  double tmp = 0;
  unsigned int i;
  int have_unknown = 0; /* true iff sl contains element not in consensus. */

  /* Can't choose exit and guard at same time */
  tor_assert(rule == NO_WEIGHTING ||
             rule == WEIGHT_FOR_EXIT ||
             rule == WEIGHT_FOR_GUARD ||
             rule == WEIGHT_FOR_MID ||
             rule == WEIGHT_FOR_DIR);

  if (smartlist_len(sl) == 0) {
    log_info(LD_CIRC,
             "Empty routerlist passed in to consensus weight node "
             "selection for rule %s",
             bandwidth_weight_rule_to_string(rule));
    return NULL;
  }

  weight_scale = circuit_build_times_get_bw_scale(NULL);

  if (rule == WEIGHT_FOR_GUARD) {
    Wg = networkstatus_get_bw_weight(NULL, "Wgg", -1);
    Wm = networkstatus_get_bw_weight(NULL, "Wgm", -1); /* Bridges */
    We = 0;
    Wd = networkstatus_get_bw_weight(NULL, "Wgd", -1);

    Wgb = networkstatus_get_bw_weight(NULL, "Wgb", -1);
    Wmb = networkstatus_get_bw_weight(NULL, "Wmb", -1);
    Web = networkstatus_get_bw_weight(NULL, "Web", -1);
    Wdb = networkstatus_get_bw_weight(NULL, "Wdb", -1);
  } else if (rule == WEIGHT_FOR_MID) {
    Wg = networkstatus_get_bw_weight(NULL, "Wmg", -1);
    Wm = networkstatus_get_bw_weight(NULL, "Wmm", -1);
    We = networkstatus_get_bw_weight(NULL, "Wme", -1);
    Wd = networkstatus_get_bw_weight(NULL, "Wmd", -1);

    Wgb = networkstatus_get_bw_weight(NULL, "Wgb", -1);
    Wmb = networkstatus_get_bw_weight(NULL, "Wmb", -1);
    Web = networkstatus_get_bw_weight(NULL, "Web", -1);
    Wdb = networkstatus_get_bw_weight(NULL, "Wdb", -1);
  } else if (rule == WEIGHT_FOR_EXIT) {
    // Guards CAN be exits if they have weird exit policies
    // They are d then I guess...
    We = networkstatus_get_bw_weight(NULL, "Wee", -1);
    Wm = networkstatus_get_bw_weight(NULL, "Wem", -1); /* Odd exit policies */
    Wd = networkstatus_get_bw_weight(NULL, "Wed", -1);
    Wg = networkstatus_get_bw_weight(NULL, "Weg", -1); /* Odd exit policies */

    Wgb = networkstatus_get_bw_weight(NULL, "Wgb", -1);
    Wmb = networkstatus_get_bw_weight(NULL, "Wmb", -1);
    Web = networkstatus_get_bw_weight(NULL, "Web", -1);
    Wdb = networkstatus_get_bw_weight(NULL, "Wdb", -1);
  } else if (rule == WEIGHT_FOR_DIR) {
    We = networkstatus_get_bw_weight(NULL, "Wbe", -1);
    Wm = networkstatus_get_bw_weight(NULL, "Wbm", -1);
    Wd = networkstatus_get_bw_weight(NULL, "Wbd", -1);
    Wg = networkstatus_get_bw_weight(NULL, "Wbg", -1);

    Wgb = Wmb = Web = Wdb = weight_scale;
  } else if (rule == NO_WEIGHTING) {
    Wg = Wm = We = Wd = weight_scale;
    Wgb = Wmb = Web = Wdb = weight_scale;
  }

  if (Wg < 0 || Wm < 0 || We < 0 || Wd < 0 || Wgb < 0 || Wmb < 0 || Wdb < 0
      || Web < 0) {
    log_debug(LD_CIRC,
              "Got negative bandwidth weights. Defaulting to old selection"
              " algorithm.");
    return NULL; // Use old algorithm.
  }

  Wg /= weight_scale;
  Wm /= weight_scale;
  We /= weight_scale;
  Wd /= weight_scale;

  Wgb /= weight_scale;
  Wmb /= weight_scale;
  Web /= weight_scale;
  Wdb /= weight_scale;

  bandwidths = tor_malloc_zero(sizeof(double)*smartlist_len(sl));

  // Cycle through smartlist and total the bandwidth.
  for (i = 0; i < (unsigned)smartlist_len(sl); ++i) {
    int is_exit = 0, is_guard = 0, is_dir = 0, this_bw = 0, is_me = 0;
    double weight = 1;
    if (statuses) {
      routerstatus_t *status = smartlist_get(sl, i);
      is_exit = status->is_exit && !status->is_bad_exit;
      is_guard = status->is_possible_guard;
      is_dir = (status->dir_port != 0);
      if (!status->has_bandwidth) {
        tor_free(bandwidths);
        /* This should never happen, unless all the authorites downgrade
         * to 0.2.0 or rogue routerstatuses get inserted into our consensus. */
        log_warn(LD_BUG,
                 "Consensus is not listing bandwidths. Defaulting back to "
                 "old router selection algorithm.");
        return NULL;
      }
      this_bw = kb_to_bytes(status->bandwidth);
      if (router_digest_is_me(status->identity_digest))
        is_me = 1;
    } else {
      routerstatus_t *rs;
      routerinfo_t *router = smartlist_get(sl, i);
      rs = router_get_consensus_status_by_id(
             router->cache_info.identity_digest);
      is_exit = router->is_exit && !router->is_bad_exit;
      is_guard = router->is_possible_guard;
      is_dir = (router->dir_port != 0);
      if (rs && rs->has_bandwidth) {
        this_bw = kb_to_bytes(rs->bandwidth);
      } else { /* bridge or other descriptor not in our consensus */
        this_bw = bridge_get_advertised_bandwidth_bounded(router);
        have_unknown = 1;
      }
      if (router_digest_is_me(router->cache_info.identity_digest))
        is_me = 1;
    }
    if (is_guard && is_exit) {
      weight = (is_dir ? Wdb*Wd : Wd);
    } else if (is_guard) {
      weight = (is_dir ? Wgb*Wg : Wg);
    } else if (is_exit) {
      weight = (is_dir ? Web*We : We);
    } else { // middle
      weight = (is_dir ? Wmb*Wm : Wm);
    }

    bandwidths[i] = weight*this_bw;
    weighted_bw += weight*this_bw;
    if (is_me)
      sl_last_weighted_bw_of_me = weight*this_bw;
  }

  /* XXXX023 this is a kludge to expose these values. */
  sl_last_total_weighted_bw = weighted_bw;

  log_debug(LD_CIRC, "Choosing node for rule %s based on weights "
            "Wg=%f Wm=%f We=%f Wd=%f with total bw %f",
            bandwidth_weight_rule_to_string(rule),
            Wg, Wm, We, Wd, weighted_bw);

  /* If there is no bandwidth, choose at random */
  if (DBL_TO_U64(weighted_bw) == 0) {
    /* Don't warn when using bridges/relays not in the consensus */
    if (!have_unknown)
      log_warn(LD_CIRC,
               "Weighted bandwidth is %f in node selection for rule %s",
               weighted_bw, bandwidth_weight_rule_to_string(rule));
    tor_free(bandwidths);
    return smartlist_choose(sl);
  }

  rand_bw = crypto_rand_uint64(DBL_TO_U64(weighted_bw));
  rand_bw++; /* crypto_rand_uint64() counts from 0, and we need to count
              * from 1 below. See bug 1203 for details. */

  /* Last, count through sl until we get to the element we picked */
  tmp = 0.0;
  for (i=0; i < (unsigned)smartlist_len(sl); i++) {
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
             "%f " U64_FORMAT " %f", tmp, U64_PRINTF_ARG(rand_bw),
             weighted_bw);
  }
  tor_free(bandwidths);
  return smartlist_get(sl, i);
}