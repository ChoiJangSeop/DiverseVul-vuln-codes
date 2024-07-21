int iscsi_host_get_param(struct Scsi_Host *shost, enum iscsi_host_param param,
			 char *buf)
{
	struct iscsi_host *ihost = shost_priv(shost);
	int len;

	switch (param) {
	case ISCSI_HOST_PARAM_NETDEV_NAME:
		len = sprintf(buf, "%s\n", ihost->netdev);
		break;
	case ISCSI_HOST_PARAM_HWADDRESS:
		len = sprintf(buf, "%s\n", ihost->hwaddress);
		break;
	case ISCSI_HOST_PARAM_INITIATOR_NAME:
		len = sprintf(buf, "%s\n", ihost->initiatorname);
		break;
	default:
		return -ENOSYS;
	}

	return len;
}