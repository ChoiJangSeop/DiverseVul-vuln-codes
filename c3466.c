static void tcp_cwnd_reduction(struct sock *sk, const int prior_unsacked,
			       int fast_rexmit, int flag)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int sndcnt = 0;
	int delta = tp->snd_ssthresh - tcp_packets_in_flight(tp);
	int newly_acked_sacked = prior_unsacked -
				 (tp->packets_out - tp->sacked_out);

	tp->prr_delivered += newly_acked_sacked;
	if (delta < 0) {
		u64 dividend = (u64)tp->snd_ssthresh * tp->prr_delivered +
			       tp->prior_cwnd - 1;
		sndcnt = div_u64(dividend, tp->prior_cwnd) - tp->prr_out;
	} else if ((flag & FLAG_RETRANS_DATA_ACKED) &&
		   !(flag & FLAG_LOST_RETRANS)) {
		sndcnt = min_t(int, delta,
			       max_t(int, tp->prr_delivered - tp->prr_out,
				     newly_acked_sacked) + 1);
	} else {
		sndcnt = min(delta, newly_acked_sacked);
	}
	sndcnt = max(sndcnt, (fast_rexmit ? 1 : 0));
	tp->snd_cwnd = tcp_packets_in_flight(tp) + sndcnt;
}