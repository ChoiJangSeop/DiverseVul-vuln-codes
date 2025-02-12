static void sas_revalidate_domain(struct work_struct *work)
{
	int res = 0;
	struct sas_discovery_event *ev = to_sas_discovery_event(work);
	struct asd_sas_port *port = ev->port;
	struct sas_ha_struct *ha = port->ha;
	struct domain_device *ddev = port->port_dev;

	/* prevent revalidation from finding sata links in recovery */
	mutex_lock(&ha->disco_mutex);
	if (test_bit(SAS_HA_ATA_EH_ACTIVE, &ha->state)) {
		SAS_DPRINTK("REVALIDATION DEFERRED on port %d, pid:%d\n",
			    port->id, task_pid_nr(current));
		goto out;
	}

	clear_bit(DISCE_REVALIDATE_DOMAIN, &port->disc.pending);

	SAS_DPRINTK("REVALIDATING DOMAIN on port %d, pid:%d\n", port->id,
		    task_pid_nr(current));

	if (ddev && (ddev->dev_type == SAS_FANOUT_EXPANDER_DEVICE ||
		     ddev->dev_type == SAS_EDGE_EXPANDER_DEVICE))
		res = sas_ex_revalidate_domain(ddev);

	SAS_DPRINTK("done REVALIDATING DOMAIN on port %d, pid:%d, res 0x%x\n",
		    port->id, task_pid_nr(current), res);
 out:
	mutex_unlock(&ha->disco_mutex);
}