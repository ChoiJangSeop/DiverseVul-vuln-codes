void hid_debug_event(struct hid_device *hdev, char *buf)
{
	unsigned i;
	struct hid_debug_list *list;
	unsigned long flags;

	spin_lock_irqsave(&hdev->debug_list_lock, flags);
	list_for_each_entry(list, &hdev->debug_list, node) {
		for (i = 0; buf[i]; i++)
			list->hid_debug_buf[(list->tail + i) % HID_DEBUG_BUFSIZE] =
				buf[i];
		list->tail = (list->tail + i) % HID_DEBUG_BUFSIZE;
        }
	spin_unlock_irqrestore(&hdev->debug_list_lock, flags);

	wake_up_interruptible(&hdev->debug_wait);
}