int xenvif_kthread_guest_rx(void *data)
{
	struct xenvif *vif = data;
	struct sk_buff *skb;

	while (!kthread_should_stop()) {
		wait_event_interruptible(vif->wq,
					 rx_work_todo(vif) ||
					 kthread_should_stop());
		if (kthread_should_stop())
			break;

		if (vif->rx_queue_purge) {
			skb_queue_purge(&vif->rx_queue);
			vif->rx_queue_purge = false;
		}

		if (!skb_queue_empty(&vif->rx_queue))
			xenvif_rx_action(vif);

		if (skb_queue_empty(&vif->rx_queue) &&
		    netif_queue_stopped(vif->dev)) {
			del_timer_sync(&vif->wake_queue);
			xenvif_start_queue(vif);
		}

		cond_resched();
	}

	/* Bin any remaining skbs */
	while ((skb = skb_dequeue(&vif->rx_queue)) != NULL)
		dev_kfree_skb(skb);

	return 0;
}