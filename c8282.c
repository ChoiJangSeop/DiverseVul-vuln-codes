static int htc_setup_complete(struct htc_target *target)
{
	struct sk_buff *skb;
	struct htc_comp_msg *comp_msg;
	int ret = 0;
	unsigned long time_left;

	skb = alloc_skb(50 + sizeof(struct htc_frame_hdr), GFP_ATOMIC);
	if (!skb) {
		dev_err(target->dev, "failed to allocate send buffer\n");
		return -ENOMEM;
	}
	skb_reserve(skb, sizeof(struct htc_frame_hdr));

	comp_msg = skb_put(skb, sizeof(struct htc_comp_msg));
	comp_msg->msg_id = cpu_to_be16(HTC_MSG_SETUP_COMPLETE_ID);

	target->htc_flags |= HTC_OP_START_WAIT;

	ret = htc_issue_send(target, skb, skb->len, 0, ENDPOINT0);
	if (ret)
		goto err;

	time_left = wait_for_completion_timeout(&target->cmd_wait, HZ);
	if (!time_left) {
		dev_err(target->dev, "HTC start timeout\n");
		return -ETIMEDOUT;
	}

	return 0;

err:
	kfree_skb(skb);
	return -EINVAL;
}