static int eb_copy_relocations(const struct i915_execbuffer *eb)
{
	const unsigned int count = eb->buffer_count;
	unsigned int i;
	int err;

	for (i = 0; i < count; i++) {
		const unsigned int nreloc = eb->exec[i].relocation_count;
		struct drm_i915_gem_relocation_entry __user *urelocs;
		struct drm_i915_gem_relocation_entry *relocs;
		unsigned long size;
		unsigned long copied;

		if (nreloc == 0)
			continue;

		err = check_relocations(&eb->exec[i]);
		if (err)
			goto err;

		urelocs = u64_to_user_ptr(eb->exec[i].relocs_ptr);
		size = nreloc * sizeof(*relocs);

		relocs = kvmalloc_array(size, 1, GFP_KERNEL);
		if (!relocs) {
			err = -ENOMEM;
			goto err;
		}

		/* copy_from_user is limited to < 4GiB */
		copied = 0;
		do {
			unsigned int len =
				min_t(u64, BIT_ULL(31), size - copied);

			if (__copy_from_user((char *)relocs + copied,
					     (char __user *)urelocs + copied,
					     len)) {
end_user:
				user_access_end();
				kvfree(relocs);
				err = -EFAULT;
				goto err;
			}

			copied += len;
		} while (copied < size);

		/*
		 * As we do not update the known relocation offsets after
		 * relocating (due to the complexities in lock handling),
		 * we need to mark them as invalid now so that we force the
		 * relocation processing next time. Just in case the target
		 * object is evicted and then rebound into its old
		 * presumed_offset before the next execbuffer - if that
		 * happened we would make the mistake of assuming that the
		 * relocations were valid.
		 */
		user_access_begin();
		for (copied = 0; copied < nreloc; copied++)
			unsafe_put_user(-1,
					&urelocs[copied].presumed_offset,
					end_user);
		user_access_end();

		eb->exec[i].relocs_ptr = (uintptr_t)relocs;
	}

	return 0;

err:
	while (i--) {
		struct drm_i915_gem_relocation_entry *relocs =
			u64_to_ptr(typeof(*relocs), eb->exec[i].relocs_ptr);
		if (eb->exec[i].relocation_count)
			kvfree(relocs);
	}
	return err;
}