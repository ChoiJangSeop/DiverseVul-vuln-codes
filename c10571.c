static void ucma_lock_files(struct ucma_file *file1, struct ucma_file *file2)
{
	/* Acquire mutex's based on pointer comparison to prevent deadlock. */
	if (file1 < file2) {
		mutex_lock(&file1->mut);
		mutex_lock_nested(&file2->mut, SINGLE_DEPTH_NESTING);
	} else {
		mutex_lock(&file2->mut);
		mutex_lock_nested(&file1->mut, SINGLE_DEPTH_NESTING);
	}
}