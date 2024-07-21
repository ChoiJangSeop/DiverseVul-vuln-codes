static void ml_ff_destroy(struct ff_device *ff)
{
	struct ml_device *ml = ff->private;

	kfree(ml->private);
}