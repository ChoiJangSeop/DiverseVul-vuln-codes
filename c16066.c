static int atl2_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct net_device *netdev;
	struct atl2_adapter *adapter;
	static int cards_found;
	unsigned long mmio_start;
	int mmio_len;
	int err;

	cards_found = 0;

	err = pci_enable_device(pdev);
	if (err)
		return err;

	/*
	 * atl2 is a shared-high-32-bit device, so we're stuck with 32-bit DMA
	 * until the kernel has the proper infrastructure to support 64-bit DMA
	 * on these devices.
	 */
	if (pci_set_dma_mask(pdev, DMA_BIT_MASK(32)) &&
		pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32))) {
		printk(KERN_ERR "atl2: No usable DMA configuration, aborting\n");
		goto err_dma;
	}

	/* Mark all PCI regions associated with PCI device
	 * pdev as being reserved by owner atl2_driver_name */
	err = pci_request_regions(pdev, atl2_driver_name);
	if (err)
		goto err_pci_reg;

	/* Enables bus-mastering on the device and calls
	 * pcibios_set_master to do the needed arch specific settings */
	pci_set_master(pdev);

	err = -ENOMEM;
	netdev = alloc_etherdev(sizeof(struct atl2_adapter));
	if (!netdev)
		goto err_alloc_etherdev;

	SET_NETDEV_DEV(netdev, &pdev->dev);

	pci_set_drvdata(pdev, netdev);
	adapter = netdev_priv(netdev);
	adapter->netdev = netdev;
	adapter->pdev = pdev;
	adapter->hw.back = adapter;

	mmio_start = pci_resource_start(pdev, 0x0);
	mmio_len = pci_resource_len(pdev, 0x0);

	adapter->hw.mem_rang = (u32)mmio_len;
	adapter->hw.hw_addr = ioremap(mmio_start, mmio_len);
	if (!adapter->hw.hw_addr) {
		err = -EIO;
		goto err_ioremap;
	}

	atl2_setup_pcicmd(pdev);

	netdev->netdev_ops = &atl2_netdev_ops;
	netdev->ethtool_ops = &atl2_ethtool_ops;
	netdev->watchdog_timeo = 5 * HZ;
	strncpy(netdev->name, pci_name(pdev), sizeof(netdev->name) - 1);

	netdev->mem_start = mmio_start;
	netdev->mem_end = mmio_start + mmio_len;
	adapter->bd_number = cards_found;
	adapter->pci_using_64 = false;

	/* setup the private structure */
	err = atl2_sw_init(adapter);
	if (err)
		goto err_sw_init;

	err = -EIO;

	netdev->hw_features = NETIF_F_SG | NETIF_F_HW_VLAN_CTAG_RX;
	netdev->features |= (NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_CTAG_RX);

	/* Init PHY as early as possible due to power saving issue  */
	atl2_phy_init(&adapter->hw);

	/* reset the controller to
	 * put the device in a known good starting state */

	if (atl2_reset_hw(&adapter->hw)) {
		err = -EIO;
		goto err_reset;
	}

	/* copy the MAC address out of the EEPROM */
	atl2_read_mac_addr(&adapter->hw);
	memcpy(netdev->dev_addr, adapter->hw.mac_addr, netdev->addr_len);
	if (!is_valid_ether_addr(netdev->dev_addr)) {
		err = -EIO;
		goto err_eeprom;
	}

	atl2_check_options(adapter);

	setup_timer(&adapter->watchdog_timer, atl2_watchdog,
		    (unsigned long)adapter);

	setup_timer(&adapter->phy_config_timer, atl2_phy_config,
		    (unsigned long)adapter);

	INIT_WORK(&adapter->reset_task, atl2_reset_task);
	INIT_WORK(&adapter->link_chg_task, atl2_link_chg_task);

	strcpy(netdev->name, "eth%d"); /* ?? */
	err = register_netdev(netdev);
	if (err)
		goto err_register;

	/* assume we have no link for now */
	netif_carrier_off(netdev);
	netif_stop_queue(netdev);

	cards_found++;

	return 0;

err_reset:
err_register:
err_sw_init:
err_eeprom:
	iounmap(adapter->hw.hw_addr);
err_ioremap:
	free_netdev(netdev);
err_alloc_etherdev:
	pci_release_regions(pdev);
err_pci_reg:
err_dma:
	pci_disable_device(pdev);
	return err;
}