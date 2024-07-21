struct sk_buff *__alloc_skb(unsigned int size, gfp_t gfp_mask,
			    int fclone)
{
	struct sk_buff *skb;
	u8 *data;

	/* Get the HEAD */
	if (fclone)
		skb = kmem_cache_alloc(skbuff_fclone_cache,
				       gfp_mask & ~__GFP_DMA);
	else
		skb = kmem_cache_alloc(skbuff_head_cache,
				       gfp_mask & ~__GFP_DMA);

	if (!skb)
		goto out;

	/* Get the DATA. Size must match skb_add_mtu(). */
	size = SKB_DATA_ALIGN(size);
	data = kmalloc(size + sizeof(struct skb_shared_info), gfp_mask);
	if (!data)
		goto nodata;

	memset(skb, 0, offsetof(struct sk_buff, truesize));
	skb->truesize = size + sizeof(struct sk_buff);
	atomic_set(&skb->users, 1);
	skb->head = data;
	skb->data = data;
	skb->tail = data;
	skb->end  = data + size;
	if (fclone) {
		struct sk_buff *child = skb + 1;
		atomic_t *fclone_ref = (atomic_t *) (child + 1);

		skb->fclone = SKB_FCLONE_ORIG;
		atomic_set(fclone_ref, 1);

		child->fclone = SKB_FCLONE_UNAVAILABLE;
	}
	atomic_set(&(skb_shinfo(skb)->dataref), 1);
	skb_shinfo(skb)->nr_frags  = 0;
	skb_shinfo(skb)->tso_size = 0;
	skb_shinfo(skb)->tso_segs = 0;
	skb_shinfo(skb)->frag_list = NULL;
out:
	return skb;
nodata:
	kmem_cache_free(skbuff_head_cache, skb);
	skb = NULL;
	goto out;
}