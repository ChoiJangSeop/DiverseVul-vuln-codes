static void ath6kl_usb_free_urb_to_pipe(struct ath6kl_usb_pipe *pipe,
					struct ath6kl_urb_context *urb_context)
{
	unsigned long flags;

	spin_lock_irqsave(&pipe->ar_usb->cs_lock, flags);
	pipe->urb_cnt++;

	list_add(&urb_context->link, &pipe->urb_list_head);
	spin_unlock_irqrestore(&pipe->ar_usb->cs_lock, flags);
}