inline int nci_request(struct nci_dev *ndev,
		       void (*req)(struct nci_dev *ndev,
				   const void *opt),
		       const void *opt, __u32 timeout)
{
	int rc;

	if (!test_bit(NCI_UP, &ndev->flags))
		return -ENETDOWN;

	/* Serialize all requests */
	mutex_lock(&ndev->req_lock);
	rc = __nci_request(ndev, req, opt, timeout);
	mutex_unlock(&ndev->req_lock);

	return rc;
}