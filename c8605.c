static int sfb_enqueue(struct sk_buff *skb, struct Qdisc *sch,
		       struct sk_buff **to_free)
{

	struct sfb_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child = q->qdisc;
	struct tcf_proto *fl;
	int i;
	u32 p_min = ~0;
	u32 minqlen = ~0;
	u32 r, sfbhash;
	u32 slot = q->slot;
	int ret = NET_XMIT_SUCCESS | __NET_XMIT_BYPASS;

	if (unlikely(sch->q.qlen >= q->limit)) {
		qdisc_qstats_overlimit(sch);
		q->stats.queuedrop++;
		goto drop;
	}

	if (q->rehash_interval > 0) {
		unsigned long limit = q->rehash_time + q->rehash_interval;

		if (unlikely(time_after(jiffies, limit))) {
			sfb_swap_slot(q);
			q->rehash_time = jiffies;
		} else if (unlikely(!q->double_buffering && q->warmup_time > 0 &&
				    time_after(jiffies, limit - q->warmup_time))) {
			q->double_buffering = true;
		}
	}

	fl = rcu_dereference_bh(q->filter_list);
	if (fl) {
		u32 salt;

		/* If using external classifiers, get result and record it. */
		if (!sfb_classify(skb, fl, &ret, &salt))
			goto other_drop;
		sfbhash = jhash_1word(salt, q->bins[slot].perturbation);
	} else {
		sfbhash = skb_get_hash_perturb(skb, q->bins[slot].perturbation);
	}


	if (!sfbhash)
		sfbhash = 1;
	sfb_skb_cb(skb)->hashes[slot] = sfbhash;

	for (i = 0; i < SFB_LEVELS; i++) {
		u32 hash = sfbhash & SFB_BUCKET_MASK;
		struct sfb_bucket *b = &q->bins[slot].bins[i][hash];

		sfbhash >>= SFB_BUCKET_SHIFT;
		if (b->qlen == 0)
			decrement_prob(b, q);
		else if (b->qlen >= q->bin_size)
			increment_prob(b, q);
		if (minqlen > b->qlen)
			minqlen = b->qlen;
		if (p_min > b->p_mark)
			p_min = b->p_mark;
	}

	slot ^= 1;
	sfb_skb_cb(skb)->hashes[slot] = 0;

	if (unlikely(minqlen >= q->max)) {
		qdisc_qstats_overlimit(sch);
		q->stats.bucketdrop++;
		goto drop;
	}

	if (unlikely(p_min >= SFB_MAX_PROB)) {
		/* Inelastic flow */
		if (q->double_buffering) {
			sfbhash = skb_get_hash_perturb(skb,
			    q->bins[slot].perturbation);
			if (!sfbhash)
				sfbhash = 1;
			sfb_skb_cb(skb)->hashes[slot] = sfbhash;

			for (i = 0; i < SFB_LEVELS; i++) {
				u32 hash = sfbhash & SFB_BUCKET_MASK;
				struct sfb_bucket *b = &q->bins[slot].bins[i][hash];

				sfbhash >>= SFB_BUCKET_SHIFT;
				if (b->qlen == 0)
					decrement_prob(b, q);
				else if (b->qlen >= q->bin_size)
					increment_prob(b, q);
			}
		}
		if (sfb_rate_limit(skb, q)) {
			qdisc_qstats_overlimit(sch);
			q->stats.penaltydrop++;
			goto drop;
		}
		goto enqueue;
	}

	r = prandom_u32() & SFB_MAX_PROB;

	if (unlikely(r < p_min)) {
		if (unlikely(p_min > SFB_MAX_PROB / 2)) {
			/* If we're marking that many packets, then either
			 * this flow is unresponsive, or we're badly congested.
			 * In either case, we want to start dropping packets.
			 */
			if (r < (p_min - SFB_MAX_PROB / 2) * 2) {
				q->stats.earlydrop++;
				goto drop;
			}
		}
		if (INET_ECN_set_ce(skb)) {
			q->stats.marked++;
		} else {
			q->stats.earlydrop++;
			goto drop;
		}
	}

enqueue:
	ret = qdisc_enqueue(skb, child, to_free);
	if (likely(ret == NET_XMIT_SUCCESS)) {
		qdisc_qstats_backlog_inc(sch, skb);
		sch->q.qlen++;
		increment_qlen(skb, q);
	} else if (net_xmit_drop_count(ret)) {
		q->stats.childdrop++;
		qdisc_qstats_drop(sch);
	}
	return ret;

drop:
	qdisc_drop(skb, sch, to_free);
	return NET_XMIT_CN;
other_drop:
	if (ret & __NET_XMIT_BYPASS)
		qdisc_qstats_drop(sch);
	kfree_skb(skb);
	return ret;
}