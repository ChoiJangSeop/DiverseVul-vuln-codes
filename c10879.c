static char *get_data(struct libmnt_fs *fs, int num)
{
	char *str = NULL;
	const char *t = NULL, *v = NULL;
	int col_id = get_column_id(num);

	switch (col_id) {
	case COL_SOURCES:
		/* print all devices with the same tag (LABEL, UUID) */
		if ((flags & FL_EVALUATE) &&
		    mnt_fs_get_tag(fs, &t, &v) == 0) {
			blkid_dev_iterate iter;
			blkid_dev dev;
			blkid_cache cache = NULL;
			struct ul_buffer buf = UL_INIT_BUFFER;
			int i = 0;

			if (blkid_get_cache(&cache, NULL) < 0)
				break;

			blkid_probe_all(cache);

			iter = blkid_dev_iterate_begin(cache);
			blkid_dev_set_search(iter, t, v);
			while (blkid_dev_next(iter, &dev) == 0) {
				dev = blkid_verify(cache, dev);
				if (!dev)
					continue;
				if (i != 0)
					ul_buffer_append_data(&buf, "\n", 1);
				ul_buffer_append_string(&buf, blkid_dev_devname(dev));
				i++;
			}
			blkid_dev_iterate_end(iter);
			str = ul_buffer_get_data(&buf, NULL, NULL);
			break;
		}
		/* fallthrough */
	case COL_SOURCE:
	{
		const char *root = mnt_fs_get_root(fs);
		const char *spec = mnt_fs_get_srcpath(fs);
		char *cn = NULL;

		if (spec && (flags & FL_CANONICALIZE))
			spec = cn = mnt_resolve_path(spec, cache);
		if (!spec) {
			spec = mnt_fs_get_source(fs);

			if (spec && (flags & FL_EVALUATE))
				spec = cn = mnt_resolve_spec(spec, cache);
		}
		if (root && spec && !(flags & FL_NOFSROOT) && strcmp(root, "/") != 0)
			xasprintf(&str, "%s[%s]", spec, root);
		else if (spec)
			str = xstrdup(spec);
		if (!cache)
			free(cn);
		break;
	}

	case COL_TARGET:
		if (mnt_fs_get_target(fs))
			str = xstrdup(mnt_fs_get_target(fs));
		break;
	case COL_FSTYPE:
		if (mnt_fs_get_fstype(fs))
			str = xstrdup(mnt_fs_get_fstype(fs));
		break;
	case COL_OPTIONS:
		if (mnt_fs_get_options(fs))
			str = xstrdup(mnt_fs_get_options(fs));
		break;
	case COL_VFS_OPTIONS:
		if (flags & FL_VFS_ALL)
			str = mnt_fs_get_vfs_options_all(fs);
		else if (mnt_fs_get_vfs_options(fs))
			str = xstrdup(mnt_fs_get_vfs_options(fs));
		break;
	case COL_FS_OPTIONS:
		if (mnt_fs_get_fs_options(fs))
			str = xstrdup(mnt_fs_get_fs_options(fs));
		break;
	case COL_OPT_FIELDS:
		if (mnt_fs_get_optional_fields(fs))
			str = xstrdup(mnt_fs_get_optional_fields(fs));
		break;
	case COL_UUID:
		str = get_tag(fs, "UUID", col_id);
		break;
	case COL_PARTUUID:
		str = get_tag(fs, "PARTUUID", col_id);
		break;
	case COL_LABEL:
		str = get_tag(fs, "LABEL", col_id);
		break;
	case COL_PARTLABEL:
		str = get_tag(fs, "PARTLABEL", col_id);
		break;

	case COL_MAJMIN:
	{
		dev_t devno = mnt_fs_get_devno(fs);
		if (!devno)
			break;

		if ((flags & FL_RAW) || (flags & FL_EXPORT) || (flags & FL_JSON))
			xasprintf(&str, "%u:%u", major(devno), minor(devno));
		else
			xasprintf(&str, "%3u:%-3u", major(devno), minor(devno));
		break;
	}
	case COL_SIZE:
	case COL_AVAIL:
	case COL_USED:
	case COL_USEPERC:
		str = get_vfs_attr(fs, col_id);
		break;
	case COL_FSROOT:
		if (mnt_fs_get_root(fs))
			str = xstrdup(mnt_fs_get_root(fs));
		break;
	case COL_TID:
		if (mnt_fs_get_tid(fs))
			xasprintf(&str, "%d", mnt_fs_get_tid(fs));
		break;
	case COL_ID:
		if (mnt_fs_get_id(fs))
			xasprintf(&str, "%d", mnt_fs_get_id(fs));
		break;
	case COL_PARENT:
		if (mnt_fs_get_parent_id(fs))
			xasprintf(&str, "%d", mnt_fs_get_parent_id(fs));
		break;
	case COL_PROPAGATION:
		if (mnt_fs_is_kernel(fs)) {
			unsigned long fl = 0;
			char *n = NULL;

			if (mnt_fs_get_propagation(fs, &fl) != 0)
				break;

			n = xstrdup((fl & MS_SHARED) ? "shared" : "private");

			if (fl & MS_SLAVE) {
				xasprintf(&str, "%s,slave", n);
				free(n);
				n = str;
			}
			if (fl & MS_UNBINDABLE) {
				xasprintf(&str, "%s,unbindable", n);
				free(n);
				n = str;
			}
			str = n;
		}
		break;
	case COL_FREQ:
		if (!mnt_fs_is_kernel(fs))
			xasprintf(&str, "%d", mnt_fs_get_freq(fs));
		break;
	case COL_PASSNO:
		if (!mnt_fs_is_kernel(fs))
			xasprintf(&str, "%d", mnt_fs_get_passno(fs));
		break;
	case COL_DELETED:
		str = xstrdup(mnt_fs_is_deleted(fs) ? "1" : "0");
		break;
	default:
		break;
	}
	return str;
}