static void unpin_sdma_pages(struct sdma_mmu_node *node)
{
	if (node->npages) {
		unpin_vector_pages(node->pq->mm, node->pages, 0, node->npages);
		atomic_sub(node->npages, &node->pq->n_locked);
	}
}