int htc_connect_service(struct htc_target *target,
		     struct htc_service_connreq *service_connreq,
		     enum htc_endpoint_id *conn_rsp_epid)
{
	struct sk_buff *skb;
	struct htc_endpoint *endpoint;
	struct htc_conn_svc_msg *conn_msg;
	int ret;
	unsigned long time_left;

	/* Find an available endpoint */
	endpoint = get_next_avail_ep(target->endpoint);
	if (!endpoint) {
		dev_err(target->dev, "Endpoint is not available for service %d\n",
			service_connreq->service_id);
		return -EINVAL;
	}

	endpoint->service_id = service_connreq->service_id;
	endpoint->max_txqdepth = service_connreq->max_send_qdepth;
	endpoint->ul_pipeid = service_to_ulpipe(service_connreq->service_id);
	endpoint->dl_pipeid = service_to_dlpipe(service_connreq->service_id);
	endpoint->ep_callbacks = service_connreq->ep_callbacks;

	skb = alloc_skb(sizeof(struct htc_conn_svc_msg) +
			    sizeof(struct htc_frame_hdr), GFP_ATOMIC);
	if (!skb) {
		dev_err(target->dev, "Failed to allocate buf to send"
			"service connect req\n");
		return -ENOMEM;
	}

	skb_reserve(skb, sizeof(struct htc_frame_hdr));

	conn_msg = skb_put(skb, sizeof(struct htc_conn_svc_msg));
	conn_msg->service_id = cpu_to_be16(service_connreq->service_id);
	conn_msg->msg_id = cpu_to_be16(HTC_MSG_CONNECT_SERVICE_ID);
	conn_msg->con_flags = cpu_to_be16(service_connreq->con_flags);
	conn_msg->dl_pipeid = endpoint->dl_pipeid;
	conn_msg->ul_pipeid = endpoint->ul_pipeid;

	ret = htc_issue_send(target, skb, skb->len, 0, ENDPOINT0);
	if (ret)
		goto err;

	time_left = wait_for_completion_timeout(&target->cmd_wait, HZ);
	if (!time_left) {
		dev_err(target->dev, "Service connection timeout for: %d\n",
			service_connreq->service_id);
		kfree_skb(skb);
		return -ETIMEDOUT;
	}

	*conn_rsp_epid = target->conn_rsp_epid;
	return 0;
err:
	kfree_skb(skb);
	return ret;
}