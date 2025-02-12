static void free_user(struct kref *ref)
{
	struct ipmi_user *user = container_of(ref, struct ipmi_user, refcount);
	kfree(user);
}