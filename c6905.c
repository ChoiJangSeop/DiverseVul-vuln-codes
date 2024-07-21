static int vhost_net_rx_peek_head_len(struct vhost_net *net, struct sock *sk)
{
	struct vhost_net_virtqueue *rvq = &net->vqs[VHOST_NET_VQ_RX];
	struct vhost_net_virtqueue *nvq = &net->vqs[VHOST_NET_VQ_TX];
	struct vhost_virtqueue *vq = &nvq->vq;
	unsigned long uninitialized_var(endtime);
	int len = peek_head_len(rvq, sk);

	if (!len && vq->busyloop_timeout) {
		/* Both tx vq and rx socket were polled here */
		mutex_lock_nested(&vq->mutex, 1);
		vhost_disable_notify(&net->dev, vq);

		preempt_disable();
		endtime = busy_clock() + vq->busyloop_timeout;

		while (vhost_can_busy_poll(&net->dev, endtime) &&
		       !sk_has_rx_data(sk) &&
		       vhost_vq_avail_empty(&net->dev, vq))
			cpu_relax();

		preempt_enable();

		if (!vhost_vq_avail_empty(&net->dev, vq))
			vhost_poll_queue(&vq->poll);
		else if (unlikely(vhost_enable_notify(&net->dev, vq))) {
			vhost_disable_notify(&net->dev, vq);
			vhost_poll_queue(&vq->poll);
		}

		mutex_unlock(&vq->mutex);

		len = peek_head_len(rvq, sk);
	}

	return len;
}