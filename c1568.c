void* ipc_rcu_alloc(int size)
{
	void* out;
	/* 
	 * We prepend the allocation with the rcu struct, and
	 * workqueue if necessary (for vmalloc). 
	 */
	if (rcu_use_vmalloc(size)) {
		out = vmalloc(HDRLEN_VMALLOC + size);
		if (out) {
			out += HDRLEN_VMALLOC;
			container_of(out, struct ipc_rcu_hdr, data)->is_vmalloc = 1;
			container_of(out, struct ipc_rcu_hdr, data)->refcount = 1;
		}
	} else {
		out = kmalloc(HDRLEN_KMALLOC + size, GFP_KERNEL);
		if (out) {
			out += HDRLEN_KMALLOC;
			container_of(out, struct ipc_rcu_hdr, data)->is_vmalloc = 0;
			container_of(out, struct ipc_rcu_hdr, data)->refcount = 1;
		}
	}

	return out;
}