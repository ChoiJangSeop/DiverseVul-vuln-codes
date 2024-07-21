static void hci_le_adv_report_evt(struct hci_dev *hdev, struct sk_buff *skb)
{
	u8 num_reports = skb->data[0];
	void *ptr = &skb->data[1];

	hci_dev_lock(hdev);

	while (num_reports--) {
		struct hci_ev_le_advertising_info *ev = ptr;
		s8 rssi;

		if (ev->length <= HCI_MAX_AD_LENGTH) {
			rssi = ev->data[ev->length];
			process_adv_report(hdev, ev->evt_type, &ev->bdaddr,
					   ev->bdaddr_type, NULL, 0, rssi,
					   ev->data, ev->length);
		} else {
			bt_dev_err(hdev, "Dropping invalid advertising data");
		}

		ptr += sizeof(*ev) + ev->length + 1;
	}

	hci_dev_unlock(hdev);
}