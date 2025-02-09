void __fput_sync(struct file *file)
{
	if (atomic_long_dec_and_test(&file->f_count)) {
		struct task_struct *task = current;
		file_sb_list_del(file);
		BUG_ON(!(task->flags & PF_KTHREAD));
		__fput(file);
	}
}