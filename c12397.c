static void xcopy_pt_undepend_remotedev(struct xcopy_op *xop)
{
	struct se_device *remote_dev;

	if (xop->op_origin == XCOL_SOURCE_RECV_OP)
		remote_dev = xop->dst_dev;
	else
		remote_dev = xop->src_dev;

	pr_debug("Calling configfs_undepend_item for"
		  " remote_dev: %p remote_dev->dev_group: %p\n",
		  remote_dev, &remote_dev->dev_group.cg_item);

	target_undepend_item(&remote_dev->dev_group.cg_item);
}