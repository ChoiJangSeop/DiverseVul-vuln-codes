static int rtsx_usb_ms_drv_probe(struct platform_device *pdev)
{
	struct memstick_host *msh;
	struct rtsx_usb_ms *host;
	struct rtsx_ucr *ucr;
	int err;

	ucr = usb_get_intfdata(to_usb_interface(pdev->dev.parent));
	if (!ucr)
		return -ENXIO;

	dev_dbg(&(pdev->dev),
			"Realtek USB Memstick controller found\n");

	msh = memstick_alloc_host(sizeof(*host), &pdev->dev);
	if (!msh)
		return -ENOMEM;

	host = memstick_priv(msh);
	host->ucr = ucr;
	host->msh = msh;
	host->pdev = pdev;
	host->power_mode = MEMSTICK_POWER_OFF;
	platform_set_drvdata(pdev, host);

	mutex_init(&host->host_mutex);
	INIT_WORK(&host->handle_req, rtsx_usb_ms_handle_req);

	INIT_DELAYED_WORK(&host->poll_card, rtsx_usb_ms_poll_card);

	msh->request = rtsx_usb_ms_request;
	msh->set_param = rtsx_usb_ms_set_param;
	msh->caps = MEMSTICK_CAP_PAR4;

	pm_runtime_get_noresume(ms_dev(host));
	pm_runtime_set_active(ms_dev(host));
	pm_runtime_enable(ms_dev(host));

	err = memstick_add_host(msh);
	if (err)
		goto err_out;

	pm_runtime_put(ms_dev(host));

	return 0;
err_out:
	memstick_free_host(msh);
	pm_runtime_disable(ms_dev(host));
	pm_runtime_put_noidle(ms_dev(host));
	return err;
}