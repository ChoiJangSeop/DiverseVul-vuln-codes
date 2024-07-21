static int xenvif_poll(struct napi_struct *napi, int budget)
{
	struct xenvif *vif = container_of(napi, struct xenvif, napi);
	int work_done;

	work_done = xenvif_tx_action(vif, budget);

	if (work_done < budget) {
		int more_to_do = 0;
		unsigned long flags;

		/* It is necessary to disable IRQ before calling
		 * RING_HAS_UNCONSUMED_REQUESTS. Otherwise we might
		 * lose event from the frontend.
		 *
		 * Consider:
		 *   RING_HAS_UNCONSUMED_REQUESTS
		 *   <frontend generates event to trigger napi_schedule>
		 *   __napi_complete
		 *
		 * This handler is still in scheduled state so the
		 * event has no effect at all. After __napi_complete
		 * this handler is descheduled and cannot get
		 * scheduled again. We lose event in this case and the ring
		 * will be completely stalled.
		 */

		local_irq_save(flags);

		RING_FINAL_CHECK_FOR_REQUESTS(&vif->tx, more_to_do);
		if (!more_to_do)
			__napi_complete(napi);

		local_irq_restore(flags);
	}

	return work_done;
}