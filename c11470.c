static int u32_change(struct net *net, struct sk_buff *in_skb,
		      struct tcf_proto *tp, unsigned long base, u32 handle,
		      struct nlattr **tca, void **arg, u32 flags,
		      struct netlink_ext_ack *extack)
{
	struct tc_u_common *tp_c = tp->data;
	struct tc_u_hnode *ht;
	struct tc_u_knode *n;
	struct tc_u32_sel *s;
	struct nlattr *opt = tca[TCA_OPTIONS];
	struct nlattr *tb[TCA_U32_MAX + 1];
	u32 htid, userflags = 0;
	size_t sel_size;
	int err;

	if (!opt) {
		if (handle) {
			NL_SET_ERR_MSG_MOD(extack, "Filter handle requires options");
			return -EINVAL;
		} else {
			return 0;
		}
	}

	err = nla_parse_nested_deprecated(tb, TCA_U32_MAX, opt, u32_policy,
					  extack);
	if (err < 0)
		return err;

	if (tb[TCA_U32_FLAGS]) {
		userflags = nla_get_u32(tb[TCA_U32_FLAGS]);
		if (!tc_flags_valid(userflags)) {
			NL_SET_ERR_MSG_MOD(extack, "Invalid filter flags");
			return -EINVAL;
		}
	}

	n = *arg;
	if (n) {
		struct tc_u_knode *new;

		if (TC_U32_KEY(n->handle) == 0) {
			NL_SET_ERR_MSG_MOD(extack, "Key node id cannot be zero");
			return -EINVAL;
		}

		if ((n->flags ^ userflags) &
		    ~(TCA_CLS_FLAGS_IN_HW | TCA_CLS_FLAGS_NOT_IN_HW)) {
			NL_SET_ERR_MSG_MOD(extack, "Key node flags do not match passed flags");
			return -EINVAL;
		}

		new = u32_init_knode(net, tp, n);
		if (!new)
			return -ENOMEM;

		err = u32_set_parms(net, tp, base, new, tb,
				    tca[TCA_RATE], flags, new->flags,
				    extack);

		if (err) {
			u32_destroy_key(new, false);
			return err;
		}

		err = u32_replace_hw_knode(tp, new, flags, extack);
		if (err) {
			u32_destroy_key(new, false);
			return err;
		}

		if (!tc_in_hw(new->flags))
			new->flags |= TCA_CLS_FLAGS_NOT_IN_HW;

		u32_replace_knode(tp, tp_c, new);
		tcf_unbind_filter(tp, &n->res);
		tcf_exts_get_net(&n->exts);
		tcf_queue_work(&n->rwork, u32_delete_key_work);
		return 0;
	}

	if (tb[TCA_U32_DIVISOR]) {
		unsigned int divisor = nla_get_u32(tb[TCA_U32_DIVISOR]);

		if (!is_power_of_2(divisor)) {
			NL_SET_ERR_MSG_MOD(extack, "Divisor is not a power of 2");
			return -EINVAL;
		}
		if (divisor-- > 0x100) {
			NL_SET_ERR_MSG_MOD(extack, "Exceeded maximum 256 hash buckets");
			return -EINVAL;
		}
		if (TC_U32_KEY(handle)) {
			NL_SET_ERR_MSG_MOD(extack, "Divisor can only be used on a hash table");
			return -EINVAL;
		}
		ht = kzalloc(struct_size(ht, ht, divisor + 1), GFP_KERNEL);
		if (ht == NULL)
			return -ENOBUFS;
		if (handle == 0) {
			handle = gen_new_htid(tp->data, ht);
			if (handle == 0) {
				kfree(ht);
				return -ENOMEM;
			}
		} else {
			err = idr_alloc_u32(&tp_c->handle_idr, ht, &handle,
					    handle, GFP_KERNEL);
			if (err) {
				kfree(ht);
				return err;
			}
		}
		ht->refcnt = 1;
		ht->divisor = divisor;
		ht->handle = handle;
		ht->prio = tp->prio;
		idr_init(&ht->handle_idr);
		ht->flags = userflags;

		err = u32_replace_hw_hnode(tp, ht, userflags, extack);
		if (err) {
			idr_remove(&tp_c->handle_idr, handle);
			kfree(ht);
			return err;
		}

		RCU_INIT_POINTER(ht->next, tp_c->hlist);
		rcu_assign_pointer(tp_c->hlist, ht);
		*arg = ht;

		return 0;
	}

	if (tb[TCA_U32_HASH]) {
		htid = nla_get_u32(tb[TCA_U32_HASH]);
		if (TC_U32_HTID(htid) == TC_U32_ROOT) {
			ht = rtnl_dereference(tp->root);
			htid = ht->handle;
		} else {
			ht = u32_lookup_ht(tp->data, TC_U32_HTID(htid));
			if (!ht) {
				NL_SET_ERR_MSG_MOD(extack, "Specified hash table not found");
				return -EINVAL;
			}
		}
	} else {
		ht = rtnl_dereference(tp->root);
		htid = ht->handle;
	}

	if (ht->divisor < TC_U32_HASH(htid)) {
		NL_SET_ERR_MSG_MOD(extack, "Specified hash table buckets exceed configured value");
		return -EINVAL;
	}

	if (handle) {
		if (TC_U32_HTID(handle) && TC_U32_HTID(handle ^ htid)) {
			NL_SET_ERR_MSG_MOD(extack, "Handle specified hash table address mismatch");
			return -EINVAL;
		}
		handle = htid | TC_U32_NODE(handle);
		err = idr_alloc_u32(&ht->handle_idr, NULL, &handle, handle,
				    GFP_KERNEL);
		if (err)
			return err;
	} else
		handle = gen_new_kid(ht, htid);

	if (tb[TCA_U32_SEL] == NULL) {
		NL_SET_ERR_MSG_MOD(extack, "Selector not specified");
		err = -EINVAL;
		goto erridr;
	}

	s = nla_data(tb[TCA_U32_SEL]);
	sel_size = struct_size(s, keys, s->nkeys);
	if (nla_len(tb[TCA_U32_SEL]) < sel_size) {
		err = -EINVAL;
		goto erridr;
	}

	n = kzalloc(struct_size(n, sel.keys, s->nkeys), GFP_KERNEL);
	if (n == NULL) {
		err = -ENOBUFS;
		goto erridr;
	}

#ifdef CONFIG_CLS_U32_PERF
	n->pf = __alloc_percpu(struct_size(n->pf, kcnts, s->nkeys),
			       __alignof__(struct tc_u32_pcnt));
	if (!n->pf) {
		err = -ENOBUFS;
		goto errfree;
	}
#endif

	memcpy(&n->sel, s, sel_size);
	RCU_INIT_POINTER(n->ht_up, ht);
	n->handle = handle;
	n->fshift = s->hmask ? ffs(ntohl(s->hmask)) - 1 : 0;
	n->flags = userflags;

	err = tcf_exts_init(&n->exts, net, TCA_U32_ACT, TCA_U32_POLICE);
	if (err < 0)
		goto errout;

#ifdef CONFIG_CLS_U32_MARK
	n->pcpu_success = alloc_percpu(u32);
	if (!n->pcpu_success) {
		err = -ENOMEM;
		goto errout;
	}

	if (tb[TCA_U32_MARK]) {
		struct tc_u32_mark *mark;

		mark = nla_data(tb[TCA_U32_MARK]);
		n->val = mark->val;
		n->mask = mark->mask;
	}
#endif

	err = u32_set_parms(net, tp, base, n, tb, tca[TCA_RATE],
			    flags, n->flags, extack);
	if (err == 0) {
		struct tc_u_knode __rcu **ins;
		struct tc_u_knode *pins;

		err = u32_replace_hw_knode(tp, n, flags, extack);
		if (err)
			goto errhw;

		if (!tc_in_hw(n->flags))
			n->flags |= TCA_CLS_FLAGS_NOT_IN_HW;

		ins = &ht->ht[TC_U32_HASH(handle)];
		for (pins = rtnl_dereference(*ins); pins;
		     ins = &pins->next, pins = rtnl_dereference(*ins))
			if (TC_U32_NODE(handle) < TC_U32_NODE(pins->handle))
				break;

		RCU_INIT_POINTER(n->next, pins);
		rcu_assign_pointer(*ins, n);
		tp_c->knodes++;
		*arg = n;
		return 0;
	}

errhw:
#ifdef CONFIG_CLS_U32_MARK
	free_percpu(n->pcpu_success);
#endif

errout:
	tcf_exts_destroy(&n->exts);
#ifdef CONFIG_CLS_U32_PERF
errfree:
	free_percpu(n->pf);
#endif
	kfree(n);
erridr:
	idr_remove(&ht->handle_idr, handle);
	return err;
}