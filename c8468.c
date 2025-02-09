static int kvaser_usb_leaf_simple_cmd_async(struct kvaser_usb_net_priv *priv,
					    u8 cmd_id)
{
	struct kvaser_cmd *cmd;
	int err;

	cmd = kmalloc(sizeof(*cmd), GFP_ATOMIC);
	if (!cmd)
		return -ENOMEM;

	cmd->len = CMD_HEADER_LEN + sizeof(struct kvaser_cmd_simple);
	cmd->id = cmd_id;
	cmd->u.simple.channel = priv->channel;

	err = kvaser_usb_send_cmd_async(priv, cmd, cmd->len);
	if (err)
		kfree(cmd);

	return err;
}