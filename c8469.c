static int kvaser_usb_leaf_flush_queue(struct kvaser_usb_net_priv *priv)
{
	struct kvaser_cmd *cmd;
	int rc;

	cmd = kmalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd->id = CMD_FLUSH_QUEUE;
	cmd->len = CMD_HEADER_LEN + sizeof(struct kvaser_cmd_flush_queue);
	cmd->u.flush_queue.channel = priv->channel;
	cmd->u.flush_queue.flags = 0x00;

	rc = kvaser_usb_send_cmd(priv->dev, cmd, cmd->len);

	kfree(cmd);
	return rc;
}