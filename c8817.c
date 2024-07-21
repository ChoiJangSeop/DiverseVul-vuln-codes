static ssize_t mon_text_read_u(struct file *file, char __user *buf,
				size_t nbytes, loff_t *ppos)
{
	struct mon_reader_text *rp = file->private_data;
	struct mon_event_text *ep;
	struct mon_text_ptr ptr;

	ep = mon_text_read_wait(rp, file);
	if (IS_ERR(ep))
		return PTR_ERR(ep);
	mutex_lock(&rp->printf_lock);
	ptr.cnt = 0;
	ptr.pbuf = rp->printf_buf;
	ptr.limit = rp->printf_size;

	mon_text_read_head_u(rp, &ptr, ep);
	if (ep->type == 'E') {
		mon_text_read_statset(rp, &ptr, ep);
	} else if (ep->xfertype == USB_ENDPOINT_XFER_ISOC) {
		mon_text_read_isostat(rp, &ptr, ep);
		mon_text_read_isodesc(rp, &ptr, ep);
	} else if (ep->xfertype == USB_ENDPOINT_XFER_INT) {
		mon_text_read_intstat(rp, &ptr, ep);
	} else {
		mon_text_read_statset(rp, &ptr, ep);
	}
	ptr.cnt += snprintf(ptr.pbuf + ptr.cnt, ptr.limit - ptr.cnt,
	    " %d", ep->length);
	mon_text_read_data(rp, &ptr, ep);

	if (copy_to_user(buf, rp->printf_buf, ptr.cnt))
		ptr.cnt = -EFAULT;
	mutex_unlock(&rp->printf_lock);
	kmem_cache_free(rp->e_slab, ep);
	return ptr.cnt;
}