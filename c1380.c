static void vhost_net_ubuf_put_and_wait(struct vhost_net_ubuf_ref *ubufs)
{
	kref_put(&ubufs->kref, vhost_net_zerocopy_done_signal);
	wait_event(ubufs->wait, !atomic_read(&ubufs->kref.refcount));
	kfree(ubufs);
}