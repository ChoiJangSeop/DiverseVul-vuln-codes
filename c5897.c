static void port_show_vhci(char **out, int hub, int port, struct vhci_device *vdev)
{
	if (hub == HUB_SPEED_HIGH)
		*out += sprintf(*out, "hs  %04u %03u ",
				      port, vdev->ud.status);
	else /* hub == HUB_SPEED_SUPER */
		*out += sprintf(*out, "ss  %04u %03u ",
				      port, vdev->ud.status);

	if (vdev->ud.status == VDEV_ST_USED) {
		*out += sprintf(*out, "%03u %08x ",
				      vdev->speed, vdev->devid);
		*out += sprintf(*out, "%16p %s",
				      vdev->ud.tcp_socket,
				      dev_name(&vdev->udev->dev));

	} else {
		*out += sprintf(*out, "000 00000000 ");
		*out += sprintf(*out, "0000000000000000 0-0");
	}

	*out += sprintf(*out, "\n");
}