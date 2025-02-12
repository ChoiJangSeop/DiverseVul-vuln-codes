static int neightbl_fill_info(struct neigh_table *tbl, struct sk_buff *skb,
			      struct netlink_callback *cb)
{
	struct nlmsghdr *nlh;
	struct ndtmsg *ndtmsg;

	nlh = NLMSG_NEW_ANSWER(skb, cb, RTM_NEWNEIGHTBL, sizeof(struct ndtmsg),
			       NLM_F_MULTI);

	ndtmsg = NLMSG_DATA(nlh);

	read_lock_bh(&tbl->lock);
	ndtmsg->ndtm_family = tbl->family;

	RTA_PUT_STRING(skb, NDTA_NAME, tbl->id);
	RTA_PUT_MSECS(skb, NDTA_GC_INTERVAL, tbl->gc_interval);
	RTA_PUT_U32(skb, NDTA_THRESH1, tbl->gc_thresh1);
	RTA_PUT_U32(skb, NDTA_THRESH2, tbl->gc_thresh2);
	RTA_PUT_U32(skb, NDTA_THRESH3, tbl->gc_thresh3);

	{
		unsigned long now = jiffies;
		unsigned int flush_delta = now - tbl->last_flush;
		unsigned int rand_delta = now - tbl->last_rand;

		struct ndt_config ndc = {
			.ndtc_key_len		= tbl->key_len,
			.ndtc_entry_size	= tbl->entry_size,
			.ndtc_entries		= atomic_read(&tbl->entries),
			.ndtc_last_flush	= jiffies_to_msecs(flush_delta),
			.ndtc_last_rand		= jiffies_to_msecs(rand_delta),
			.ndtc_hash_rnd		= tbl->hash_rnd,
			.ndtc_hash_mask		= tbl->hash_mask,
			.ndtc_hash_chain_gc	= tbl->hash_chain_gc,
			.ndtc_proxy_qlen	= tbl->proxy_queue.qlen,
		};

		RTA_PUT(skb, NDTA_CONFIG, sizeof(ndc), &ndc);
	}

	{
		int cpu;
		struct ndt_stats ndst;

		memset(&ndst, 0, sizeof(ndst));

		for (cpu = 0; cpu < NR_CPUS; cpu++) {
			struct neigh_statistics	*st;

			if (!cpu_possible(cpu))
				continue;

			st = per_cpu_ptr(tbl->stats, cpu);
			ndst.ndts_allocs		+= st->allocs;
			ndst.ndts_destroys		+= st->destroys;
			ndst.ndts_hash_grows		+= st->hash_grows;
			ndst.ndts_res_failed		+= st->res_failed;
			ndst.ndts_lookups		+= st->lookups;
			ndst.ndts_hits			+= st->hits;
			ndst.ndts_rcv_probes_mcast	+= st->rcv_probes_mcast;
			ndst.ndts_rcv_probes_ucast	+= st->rcv_probes_ucast;
			ndst.ndts_periodic_gc_runs	+= st->periodic_gc_runs;
			ndst.ndts_forced_gc_runs	+= st->forced_gc_runs;
		}

		RTA_PUT(skb, NDTA_STATS, sizeof(ndst), &ndst);
	}

	BUG_ON(tbl->parms.dev);
	if (neightbl_fill_parms(skb, &tbl->parms) < 0)
		goto rtattr_failure;

	read_unlock_bh(&tbl->lock);
	return NLMSG_END(skb, nlh);

rtattr_failure:
	read_unlock_bh(&tbl->lock);
	return NLMSG_CANCEL(skb, nlh);
 
nlmsg_failure:
	return -1;
}