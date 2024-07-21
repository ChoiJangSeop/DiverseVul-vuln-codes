}

static int
iscsi_set_host_param(struct iscsi_transport *transport,
		     struct iscsi_uevent *ev)
{
	char *data = (char*)ev + sizeof(*ev);
	struct Scsi_Host *shost;
	int err;

	if (!transport->set_host_param)
		return -ENOSYS;

	shost = scsi_host_lookup(ev->u.set_host_param.host_no);
	if (!shost) {
		printk(KERN_ERR "set_host_param could not find host no %u\n",
		       ev->u.set_host_param.host_no);
		return -ENODEV;
	}

	err = transport->set_host_param(shost, ev->u.set_host_param.param,
					data, ev->u.set_host_param.len);