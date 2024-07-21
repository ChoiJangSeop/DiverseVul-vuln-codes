static int vhost_vdpa_config_validate(struct vhost_vdpa *v,
				      struct vhost_vdpa_config *c)
{
	struct vdpa_device *vdpa = v->vdpa;
	long size = vdpa->config->get_config_size(vdpa);

	if (c->len == 0 || c->off > size)
		return -EINVAL;

	if (c->len > size - c->off)
		return -E2BIG;

	return 0;
}