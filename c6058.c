int sas_smp_get_phy_events(struct sas_phy *phy)
{
	int res;
	u8 *req;
	u8 *resp;
	struct sas_rphy *rphy = dev_to_rphy(phy->dev.parent);
	struct domain_device *dev = sas_find_dev_by_rphy(rphy);

	req = alloc_smp_req(RPEL_REQ_SIZE);
	if (!req)
		return -ENOMEM;

	resp = alloc_smp_resp(RPEL_RESP_SIZE);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req[1] = SMP_REPORT_PHY_ERR_LOG;
	req[9] = phy->number;

	res = smp_execute_task(dev, req, RPEL_REQ_SIZE,
			            resp, RPEL_RESP_SIZE);

	if (!res)
		goto out;

	phy->invalid_dword_count = scsi_to_u32(&resp[12]);
	phy->running_disparity_error_count = scsi_to_u32(&resp[16]);
	phy->loss_of_dword_sync_count = scsi_to_u32(&resp[20]);
	phy->phy_reset_problem_count = scsi_to_u32(&resp[24]);

 out:
	kfree(resp);
	return res;

}