static int nci_open_device(struct nci_dev *ndev)
{
	int rc = 0;

	mutex_lock(&ndev->req_lock);

	if (test_bit(NCI_UP, &ndev->flags)) {
		rc = -EALREADY;
		goto done;
	}

	if (ndev->ops->open(ndev)) {
		rc = -EIO;
		goto done;
	}

	atomic_set(&ndev->cmd_cnt, 1);

	set_bit(NCI_INIT, &ndev->flags);

	if (ndev->ops->init)
		rc = ndev->ops->init(ndev);

	if (!rc) {
		rc = __nci_request(ndev, nci_reset_req, (void *)0,
				   msecs_to_jiffies(NCI_RESET_TIMEOUT));
	}

	if (!rc && ndev->ops->setup) {
		rc = ndev->ops->setup(ndev);
	}

	if (!rc) {
		struct nci_core_init_v2_cmd nci_init_v2_cmd = {
			.feature1 = NCI_FEATURE_DISABLE,
			.feature2 = NCI_FEATURE_DISABLE
		};
		const void *opt = NULL;

		if (ndev->nci_ver & NCI_VER_2_MASK)
			opt = &nci_init_v2_cmd;

		rc = __nci_request(ndev, nci_init_req, opt,
				   msecs_to_jiffies(NCI_INIT_TIMEOUT));
	}

	if (!rc && ndev->ops->post_setup)
		rc = ndev->ops->post_setup(ndev);

	if (!rc) {
		rc = __nci_request(ndev, nci_init_complete_req, (void *)0,
				   msecs_to_jiffies(NCI_INIT_TIMEOUT));
	}

	clear_bit(NCI_INIT, &ndev->flags);

	if (!rc) {
		set_bit(NCI_UP, &ndev->flags);
		nci_clear_target_list(ndev);
		atomic_set(&ndev->state, NCI_IDLE);
	} else {
		/* Init failed, cleanup */
		skb_queue_purge(&ndev->cmd_q);
		skb_queue_purge(&ndev->rx_q);
		skb_queue_purge(&ndev->tx_q);

		ndev->ops->close(ndev);
		ndev->flags = 0;
	}

done:
	mutex_unlock(&ndev->req_lock);
	return rc;
}