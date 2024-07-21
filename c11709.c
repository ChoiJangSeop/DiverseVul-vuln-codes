static void cgroup_pressure_release(struct kernfs_open_file *of)
{
	struct cgroup_file_ctx *ctx = of->priv;

	psi_trigger_replace(&ctx->psi.trigger, NULL);
}