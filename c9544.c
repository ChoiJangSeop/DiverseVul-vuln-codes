static void hci_le_direct_adv_report_evt(struct hci_dev *hdev,
					 struct sk_buff *skb)
{
	u8 num_reports = skb->data[0];
	void *ptr = &skb->data[1];

	hci_dev_lock(hdev);

	while (num_reports--) {
		struct hci_ev_le_direct_adv_info *ev = ptr;

		process_adv_report(hdev, ev->evt_type, &ev->bdaddr,
				   ev->bdaddr_type, &ev->direct_addr,
				   ev->direct_addr_type, ev->rssi, NULL, 0);

		ptr += sizeof(*ev);
	}

	hci_dev_unlock(hdev);
}