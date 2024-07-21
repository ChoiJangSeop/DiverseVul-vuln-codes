int igmp6_event_query(struct sk_buff *skb)
{
	struct inet6_dev *idev = __in6_dev_get(skb->dev);

	if (!idev)
		return -EINVAL;

	if (idev->dead) {
		kfree_skb(skb);
		return -ENODEV;
	}

	spin_lock_bh(&idev->mc_query_lock);
	if (skb_queue_len(&idev->mc_query_queue) < MLD_MAX_SKBS) {
		__skb_queue_tail(&idev->mc_query_queue, skb);
		if (!mod_delayed_work(mld_wq, &idev->mc_query_work, 0))
			in6_dev_hold(idev);
	}
	spin_unlock_bh(&idev->mc_query_lock);

	return 0;
}