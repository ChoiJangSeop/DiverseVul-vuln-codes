static void hci_le_ext_adv_report_evt(struct hci_dev *hdev, struct sk_buff *skb)
{
	u8 num_reports = skb->data[0];
	void *ptr = &skb->data[1];

	hci_dev_lock(hdev);

	while (num_reports--) {
		struct hci_ev_le_ext_adv_report *ev = ptr;
		u8 legacy_evt_type;
		u16 evt_type;

		evt_type = __le16_to_cpu(ev->evt_type);
		legacy_evt_type = ext_evt_type_to_legacy(hdev, evt_type);
		if (legacy_evt_type != LE_ADV_INVALID) {
			process_adv_report(hdev, legacy_evt_type, &ev->bdaddr,
					   ev->bdaddr_type, NULL, 0, ev->rssi,
					   ev->data, ev->length);
		}

		ptr += sizeof(*ev) + ev->length;
	}

	hci_dev_unlock(hdev);
}