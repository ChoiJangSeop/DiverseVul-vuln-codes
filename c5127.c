static int set_registers(pegasus_t *pegasus, __u16 indx, __u16 size, void *data)
{
	int ret;

	ret = usb_control_msg(pegasus->usb, usb_sndctrlpipe(pegasus->usb, 0),
			      PEGASUS_REQ_SET_REGS, PEGASUS_REQT_WRITE, 0,
			      indx, data, size, 100);
	if (ret < 0)
		netif_dbg(pegasus, drv, pegasus->net,
			  "%s returned %d\n", __func__, ret);
	return ret;
}