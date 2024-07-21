time_delta_should_use_heuristics(const congestion_control_t *cc)
{

  /* If we have exited slow start, we should have processed at least
   * a cwnd worth of RTTs */
  if (!cc->in_slow_start) {
    return true;
  }

  /* If we managed to get enough acks to estimate a SENDME BDP, then
   * we have enough to estimate clock jumps relative to a baseline,
   * too. (This is at least 'cc_bwe_min' acks). */
  if (cc->bdp[BDP_ALG_SENDME_RATE]) {
    return true;
  }

  /* Not enough data to estimate clock jumps */
  return false;
}