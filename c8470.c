static int kvaser_usb_leaf_set_opt_mode(const struct kvaser_usb_net_priv *priv)
{
	struct kvaser_cmd *cmd;
	int rc;

	cmd = kmalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd->id = CMD_SET_CTRL_MODE;
	cmd->len = CMD_HEADER_LEN + sizeof(struct kvaser_cmd_ctrl_mode);
	cmd->u.ctrl_mode.tid = 0xff;
	cmd->u.ctrl_mode.channel = priv->channel;

	if (priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY)
		cmd->u.ctrl_mode.ctrl_mode = KVASER_CTRL_MODE_SILENT;
	else
		cmd->u.ctrl_mode.ctrl_mode = KVASER_CTRL_MODE_NORMAL;

	rc = kvaser_usb_send_cmd(priv->dev, cmd, cmd->len);

	kfree(cmd);
	return rc;
}