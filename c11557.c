time_delta_stalled_or_jumped(const congestion_control_t *cc,
                             uint64_t old_delta, uint64_t new_delta)
{
#define DELTA_DISCREPENCY_RATIO_MAX 5000
  /* If we have a 0 new_delta, that is definitely a monotime stall */
  if (new_delta == 0) {
    static ratelim_t stall_info_limit = RATELIM_INIT(60);
    log_fn_ratelim(&stall_info_limit, LOG_INFO, LD_CIRC,
           "Congestion control cannot measure RTT due to monotime stall.");

    /* If delta is every 0, the monotime clock has stalled, and we should
     * not use it anywhere. */
    is_monotime_clock_broken = true;

    return is_monotime_clock_broken;
  }

  /* If the old_delta is 0, we have no previous values on this circuit.
   *
   * So, return the global monotime status from other circuits, and
   * do not update.
   */
  if (old_delta == 0) {
    return is_monotime_clock_broken;
  }

  /*
   * For the heuristic cases, we need at least a few timestamps,
   * to average out any previous partial stalls or jumps. So until
   * than point, let's just use the cached status from other circuits.
   */
  if (!time_delta_should_use_heuristics(cc)) {
    return is_monotime_clock_broken;
  }

  /* If old_delta is significantly larger than new_delta, then
   * this means that the monotime clock recently stopped moving
   * forward. */
  if (old_delta > new_delta * DELTA_DISCREPENCY_RATIO_MAX) {
    static ratelim_t dec_notice_limit = RATELIM_INIT(300);
    log_fn_ratelim(&dec_notice_limit, LOG_NOTICE, LD_CIRC,
           "Sudden decrease in circuit RTT (%"PRIu64" vs %"PRIu64
           "), likely due to clock jump.",
           new_delta/1000, old_delta/1000);

    is_monotime_clock_broken = true;

    return is_monotime_clock_broken;
  }

  /* If new_delta is significantly larger than old_delta, then
   * this means that the monotime clock suddenly jumped forward. */
  if (new_delta > old_delta * DELTA_DISCREPENCY_RATIO_MAX) {
    static ratelim_t dec_notice_limit = RATELIM_INIT(300);
    log_fn_ratelim(&dec_notice_limit, LOG_NOTICE, LD_CIRC,
           "Sudden increase in circuit RTT (%"PRIu64" vs %"PRIu64
           "), likely due to clock jump.",
           new_delta/1000, old_delta/1000);

    is_monotime_clock_broken = true;

    return is_monotime_clock_broken;
  }

  /* All good! Update cached status, too */
  is_monotime_clock_broken = false;

  return is_monotime_clock_broken;
}