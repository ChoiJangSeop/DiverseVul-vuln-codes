static void sas_discover_domain(struct work_struct *work)
{
	struct domain_device *dev;
	int error = 0;
	struct sas_discovery_event *ev = to_sas_discovery_event(work);
	struct asd_sas_port *port = ev->port;

	clear_bit(DISCE_DISCOVER_DOMAIN, &port->disc.pending);

	if (port->port_dev)
		return;

	error = sas_get_port_device(port);
	if (error)
		return;
	dev = port->port_dev;

	SAS_DPRINTK("DOING DISCOVERY on port %d, pid:%d\n", port->id,
		    task_pid_nr(current));

	switch (dev->dev_type) {
	case SAS_END_DEVICE:
		error = sas_discover_end_dev(dev);
		break;
	case SAS_EDGE_EXPANDER_DEVICE:
	case SAS_FANOUT_EXPANDER_DEVICE:
		error = sas_discover_root_expander(dev);
		break;
	case SAS_SATA_DEV:
	case SAS_SATA_PM:
#ifdef CONFIG_SCSI_SAS_ATA
		error = sas_discover_sata(dev);
		break;
#else
		SAS_DPRINTK("ATA device seen but CONFIG_SCSI_SAS_ATA=N so cannot attach\n");
		/* Fall through */
#endif
	default:
		error = -ENXIO;
		SAS_DPRINTK("unhandled device %d\n", dev->dev_type);
		break;
	}

	if (error) {
		sas_rphy_free(dev->rphy);
		list_del_init(&dev->disco_list_node);
		spin_lock_irq(&port->dev_list_lock);
		list_del_init(&dev->dev_list_node);
		spin_unlock_irq(&port->dev_list_lock);

		sas_put_device(dev);
		port->port_dev = NULL;
	}

	SAS_DPRINTK("DONE DISCOVERY on port %d, pid:%d, result:%d\n", port->id,
		    task_pid_nr(current), error);
}