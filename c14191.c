int ath9k_wmi_cmd(struct wmi *wmi, enum wmi_cmd_id cmd_id,
		  u8 *cmd_buf, u32 cmd_len,
		  u8 *rsp_buf, u32 rsp_len,
		  u32 timeout)
{
	struct ath_hw *ah = wmi->drv_priv->ah;
	struct ath_common *common = ath9k_hw_common(ah);
	u16 headroom = sizeof(struct htc_frame_hdr) +
		       sizeof(struct wmi_cmd_hdr);
	struct sk_buff *skb;
	unsigned long time_left;
	int ret = 0;

	if (ah->ah_flags & AH_UNPLUGGED)
		return 0;

	skb = alloc_skb(headroom + cmd_len, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;

	skb_reserve(skb, headroom);

	if (cmd_len != 0 && cmd_buf != NULL) {
		skb_put_data(skb, cmd_buf, cmd_len);
	}

	mutex_lock(&wmi->op_mutex);

	/* check if wmi stopped flag is set */
	if (unlikely(wmi->stopped)) {
		ret = -EPROTO;
		goto out;
	}

	/* record the rsp buffer and length */
	wmi->cmd_rsp_buf = rsp_buf;
	wmi->cmd_rsp_len = rsp_len;

	ret = ath9k_wmi_cmd_issue(wmi, skb, cmd_id, cmd_len);
	if (ret)
		goto out;

	time_left = wait_for_completion_timeout(&wmi->cmd_wait, timeout);
	if (!time_left) {
		ath_dbg(common, WMI, "Timeout waiting for WMI command: %s\n",
			wmi_cmd_to_name(cmd_id));
		mutex_unlock(&wmi->op_mutex);
		kfree_skb(skb);
		return -ETIMEDOUT;
	}

	mutex_unlock(&wmi->op_mutex);

	return 0;

out:
	ath_dbg(common, WMI, "WMI failure for: %s\n", wmi_cmd_to_name(cmd_id));
	mutex_unlock(&wmi->op_mutex);
	kfree_skb(skb);

	return ret;
}