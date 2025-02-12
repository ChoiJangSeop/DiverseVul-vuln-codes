static void sas_destruct_devices(struct work_struct *work)
{
	struct domain_device *dev, *n;
	struct sas_discovery_event *ev = to_sas_discovery_event(work);
	struct asd_sas_port *port = ev->port;

	clear_bit(DISCE_DESTRUCT, &port->disc.pending);

	list_for_each_entry_safe(dev, n, &port->destroy_list, disco_list_node) {
		list_del_init(&dev->disco_list_node);

		sas_remove_children(&dev->rphy->dev);
		sas_rphy_delete(dev->rphy);
		sas_unregister_common_dev(port, dev);
	}
}