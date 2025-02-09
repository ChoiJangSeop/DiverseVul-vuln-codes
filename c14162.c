static void xen_netbk_idx_release(struct xen_netbk *netbk, u16 pending_idx)
{
	struct xenvif *vif;
	struct pending_tx_info *pending_tx_info;
	pending_ring_idx_t index;

	/* Already complete? */
	if (netbk->mmap_pages[pending_idx] == NULL)
		return;

	pending_tx_info = &netbk->pending_tx_info[pending_idx];

	vif = pending_tx_info->vif;

	make_tx_response(vif, &pending_tx_info->req, XEN_NETIF_RSP_OKAY);

	index = pending_index(netbk->pending_prod++);
	netbk->pending_ring[index] = pending_idx;

	xenvif_put(vif);

	netbk->mmap_pages[pending_idx]->mapping = 0;
	put_page(netbk->mmap_pages[pending_idx]);
	netbk->mmap_pages[pending_idx] = NULL;
}