vhost_user_msg_handler(int vid, int fd)
{
	struct virtio_net *dev;
	struct VhostUserMsg msg;
	struct rte_vdpa_device *vdpa_dev;
	int did = -1;
	int ret;
	int unlock_required = 0;
	bool handled;
	int request;

	dev = get_device(vid);
	if (dev == NULL)
		return -1;

	if (!dev->notify_ops) {
		dev->notify_ops = vhost_driver_callback_get(dev->ifname);
		if (!dev->notify_ops) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"failed to get callback ops for driver %s\n",
				dev->ifname);
			return -1;
		}
	}

	ret = read_vhost_message(fd, &msg);
	if (ret <= 0) {
		if (ret < 0)
			RTE_LOG(ERR, VHOST_CONFIG,
				"vhost read message failed\n");
		else
			RTE_LOG(INFO, VHOST_CONFIG,
				"vhost peer closed\n");

		return -1;
	}

	ret = 0;
	request = msg.request.master;
	if (request > VHOST_USER_NONE && request < VHOST_USER_MAX &&
			vhost_message_str[request]) {
		if (request != VHOST_USER_IOTLB_MSG)
			RTE_LOG(INFO, VHOST_CONFIG, "read message %s\n",
				vhost_message_str[request]);
		else
			RTE_LOG(DEBUG, VHOST_CONFIG, "read message %s\n",
				vhost_message_str[request]);
	} else {
		RTE_LOG(DEBUG, VHOST_CONFIG, "External request %d\n", request);
	}

	ret = vhost_user_check_and_alloc_queue_pair(dev, &msg);
	if (ret < 0) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"failed to alloc queue\n");
		return -1;
	}

	/*
	 * Note: we don't lock all queues on VHOST_USER_GET_VRING_BASE
	 * and VHOST_USER_RESET_OWNER, since it is sent when virtio stops
	 * and device is destroyed. destroy_device waits for queues to be
	 * inactive, so it is safe. Otherwise taking the access_lock
	 * would cause a dead lock.
	 */
	switch (request) {
	case VHOST_USER_SET_FEATURES:
	case VHOST_USER_SET_PROTOCOL_FEATURES:
	case VHOST_USER_SET_OWNER:
	case VHOST_USER_SET_MEM_TABLE:
	case VHOST_USER_SET_LOG_BASE:
	case VHOST_USER_SET_LOG_FD:
	case VHOST_USER_SET_VRING_NUM:
	case VHOST_USER_SET_VRING_ADDR:
	case VHOST_USER_SET_VRING_BASE:
	case VHOST_USER_SET_VRING_KICK:
	case VHOST_USER_SET_VRING_CALL:
	case VHOST_USER_SET_VRING_ERR:
	case VHOST_USER_SET_VRING_ENABLE:
	case VHOST_USER_SEND_RARP:
	case VHOST_USER_NET_SET_MTU:
	case VHOST_USER_SET_SLAVE_REQ_FD:
		vhost_user_lock_all_queue_pairs(dev);
		unlock_required = 1;
		break;
	default:
		break;

	}

	handled = false;
	if (dev->extern_ops.pre_msg_handle) {
		ret = (*dev->extern_ops.pre_msg_handle)(dev->vid,
				(void *)&msg);
		switch (ret) {
		case RTE_VHOST_MSG_RESULT_REPLY:
			send_vhost_reply(fd, &msg);
			/* Fall-through */
		case RTE_VHOST_MSG_RESULT_ERR:
		case RTE_VHOST_MSG_RESULT_OK:
			handled = true;
			goto skip_to_post_handle;
		case RTE_VHOST_MSG_RESULT_NOT_HANDLED:
		default:
			break;
		}
	}

	if (request > VHOST_USER_NONE && request < VHOST_USER_MAX) {
		if (!vhost_message_handlers[request])
			goto skip_to_post_handle;
		ret = vhost_message_handlers[request](&dev, &msg, fd);

		switch (ret) {
		case RTE_VHOST_MSG_RESULT_ERR:
			RTE_LOG(ERR, VHOST_CONFIG,
				"Processing %s failed.\n",
				vhost_message_str[request]);
			handled = true;
			break;
		case RTE_VHOST_MSG_RESULT_OK:
			RTE_LOG(DEBUG, VHOST_CONFIG,
				"Processing %s succeeded.\n",
				vhost_message_str[request]);
			handled = true;
			break;
		case RTE_VHOST_MSG_RESULT_REPLY:
			RTE_LOG(DEBUG, VHOST_CONFIG,
				"Processing %s succeeded and needs reply.\n",
				vhost_message_str[request]);
			send_vhost_reply(fd, &msg);
			handled = true;
			break;
		default:
			break;
		}
	}

skip_to_post_handle:
	if (ret != RTE_VHOST_MSG_RESULT_ERR &&
			dev->extern_ops.post_msg_handle) {
		ret = (*dev->extern_ops.post_msg_handle)(dev->vid,
				(void *)&msg);
		switch (ret) {
		case RTE_VHOST_MSG_RESULT_REPLY:
			send_vhost_reply(fd, &msg);
			/* Fall-through */
		case RTE_VHOST_MSG_RESULT_ERR:
		case RTE_VHOST_MSG_RESULT_OK:
			handled = true;
		case RTE_VHOST_MSG_RESULT_NOT_HANDLED:
		default:
			break;
		}
	}

	if (unlock_required)
		vhost_user_unlock_all_queue_pairs(dev);

	/* If message was not handled at this stage, treat it as an error */
	if (!handled) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"vhost message (req: %d) was not handled.\n", request);
		ret = RTE_VHOST_MSG_RESULT_ERR;
	}

	/*
	 * If the request required a reply that was already sent,
	 * this optional reply-ack won't be sent as the
	 * VHOST_USER_NEED_REPLY was cleared in send_vhost_reply().
	 */
	if (msg.flags & VHOST_USER_NEED_REPLY) {
		msg.payload.u64 = ret == RTE_VHOST_MSG_RESULT_ERR;
		msg.size = sizeof(msg.payload.u64);
		msg.fd_num = 0;
		send_vhost_reply(fd, &msg);
	} else if (ret == RTE_VHOST_MSG_RESULT_ERR) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"vhost message handling failed.\n");
		return -1;
	}

	if (!(dev->flags & VIRTIO_DEV_RUNNING) && virtio_is_ready(dev)) {
		dev->flags |= VIRTIO_DEV_READY;

		if (!(dev->flags & VIRTIO_DEV_RUNNING)) {
			if (dev->dequeue_zero_copy) {
				RTE_LOG(INFO, VHOST_CONFIG,
						"dequeue zero copy is enabled\n");
			}

			if (dev->notify_ops->new_device(dev->vid) == 0)
				dev->flags |= VIRTIO_DEV_RUNNING;
		}
	}

	did = dev->vdpa_dev_id;
	vdpa_dev = rte_vdpa_get_device(did);
	if (vdpa_dev && virtio_is_ready(dev) &&
			!(dev->flags & VIRTIO_DEV_VDPA_CONFIGURED) &&
			msg.request.master == VHOST_USER_SET_VRING_CALL) {
		if (vdpa_dev->ops->dev_conf)
			vdpa_dev->ops->dev_conf(dev->vid);
		dev->flags |= VIRTIO_DEV_VDPA_CONFIGURED;
	}

	return 0;
}