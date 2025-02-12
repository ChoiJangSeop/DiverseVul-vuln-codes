int vmw_gb_surface_define_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	struct vmw_private *dev_priv = vmw_priv(dev);
	struct vmw_user_surface *user_srf;
	struct vmw_surface *srf;
	struct vmw_resource *res;
	struct vmw_resource *tmp;
	union drm_vmw_gb_surface_create_arg *arg =
	    (union drm_vmw_gb_surface_create_arg *)data;
	struct drm_vmw_gb_surface_create_req *req = &arg->req;
	struct drm_vmw_gb_surface_create_rep *rep = &arg->rep;
	struct ttm_object_file *tfile = vmw_fpriv(file_priv)->tfile;
	int ret;
	uint32_t size;
	uint32_t backup_handle;

	if (req->multisample_count != 0)
		return -EINVAL;

	if (unlikely(vmw_user_surface_size == 0))
		vmw_user_surface_size = ttm_round_pot(sizeof(*user_srf)) +
			128;

	size = vmw_user_surface_size + 128;

	/* Define a surface based on the parameters. */
	ret = vmw_surface_gb_priv_define(dev,
			size,
			req->svga3d_flags,
			req->format,
			req->drm_surface_flags & drm_vmw_surface_flag_scanout,
			req->mip_levels,
			req->multisample_count,
			req->array_size,
			req->base_size,
			&srf);
	if (unlikely(ret != 0))
		return ret;

	user_srf = container_of(srf, struct vmw_user_surface, srf);
	if (drm_is_primary_client(file_priv))
		user_srf->master = drm_master_get(file_priv->master);

	ret = ttm_read_lock(&dev_priv->reservation_sem, true);
	if (unlikely(ret != 0))
		return ret;

	res = &user_srf->srf.res;


	if (req->buffer_handle != SVGA3D_INVALID_ID) {
		ret = vmw_user_dmabuf_lookup(tfile, req->buffer_handle,
					     &res->backup,
					     &user_srf->backup_base);
		if (ret == 0 && res->backup->base.num_pages * PAGE_SIZE <
		    res->backup_size) {
			DRM_ERROR("Surface backup buffer is too small.\n");
			vmw_dmabuf_unreference(&res->backup);
			ret = -EINVAL;
			goto out_unlock;
		}
	} else if (req->drm_surface_flags & drm_vmw_surface_flag_create_buffer)
		ret = vmw_user_dmabuf_alloc(dev_priv, tfile,
					    res->backup_size,
					    req->drm_surface_flags &
					    drm_vmw_surface_flag_shareable,
					    &backup_handle,
					    &res->backup,
					    &user_srf->backup_base);

	if (unlikely(ret != 0)) {
		vmw_resource_unreference(&res);
		goto out_unlock;
	}

	tmp = vmw_resource_reference(res);
	ret = ttm_prime_object_init(tfile, res->backup_size, &user_srf->prime,
				    req->drm_surface_flags &
				    drm_vmw_surface_flag_shareable,
				    VMW_RES_SURFACE,
				    &vmw_user_surface_base_release, NULL);

	if (unlikely(ret != 0)) {
		vmw_resource_unreference(&tmp);
		vmw_resource_unreference(&res);
		goto out_unlock;
	}

	rep->handle      = user_srf->prime.base.hash.key;
	rep->backup_size = res->backup_size;
	if (res->backup) {
		rep->buffer_map_handle =
			drm_vma_node_offset_addr(&res->backup->base.vma_node);
		rep->buffer_size = res->backup->base.num_pages * PAGE_SIZE;
		rep->buffer_handle = backup_handle;
	} else {
		rep->buffer_map_handle = 0;
		rep->buffer_size = 0;
		rep->buffer_handle = SVGA3D_INVALID_ID;
	}

	vmw_resource_unreference(&res);

out_unlock:
	ttm_read_unlock(&dev_priv->reservation_sem);
	return ret;
}