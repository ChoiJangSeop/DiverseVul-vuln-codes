void nci_unregister_device(struct nci_dev *ndev)
{
	struct nci_conn_info *conn_info, *n;

	nci_close_device(ndev);

	destroy_workqueue(ndev->cmd_wq);
	destroy_workqueue(ndev->rx_wq);
	destroy_workqueue(ndev->tx_wq);

	list_for_each_entry_safe(conn_info, n, &ndev->conn_info_list, list) {
		list_del(&conn_info->list);
		/* conn_info is allocated with devm_kzalloc */
	}

	nfc_unregister_device(ndev->nfc_dev);
}