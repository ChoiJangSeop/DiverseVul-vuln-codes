static void vhost_vdpa_config_put(struct vhost_vdpa *v)
{
	if (v->config_ctx)
		eventfd_ctx_put(v->config_ctx);
}