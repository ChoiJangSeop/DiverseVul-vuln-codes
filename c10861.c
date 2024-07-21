DEFINE_TEST(test_write_disk_fixup)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
	skipping("Skipping test on Windows");
#else
	struct archive *ad;
	struct archive_entry *ae;
	int r;

	if (!canSymlink()) {
		skipping("Symlinks not supported");
		return;
	}

	/* Write entries to disk. */
	assert((ad = archive_write_disk_new()) != NULL);

	/*
	 * Create a file
	 */
	assertMakeFile("victim", 0600, "a");

	/*
	 * Create a directory and a symlink with the same name
	 */

	/* Directory: dir */
        assert((ae = archive_entry_new()) != NULL);
        archive_entry_copy_pathname(ae, "dir");
        archive_entry_set_mode(ae, AE_IFDIR | 0606);
	assertEqualIntA(ad, 0, archive_write_header(ad, ae));
	assertEqualIntA(ad, 0, archive_write_finish_entry(ad));
        archive_entry_free(ae);

	/* Symbolic Link: dir -> foo */
	assert((ae = archive_entry_new()) != NULL);
	archive_entry_copy_pathname(ae, "dir");
	archive_entry_set_mode(ae, AE_IFLNK | 0777);
	archive_entry_set_size(ae, 0);
	archive_entry_copy_symlink(ae, "victim");
	assertEqualIntA(ad, 0, r = archive_write_header(ad, ae));
	if (r >= ARCHIVE_WARN)
		assertEqualIntA(ad, 0, archive_write_finish_entry(ad));
	archive_entry_free(ae);

	assertEqualInt(ARCHIVE_OK, archive_write_free(ad));

	/* Test the entries on disk. */
	assertIsSymlink("dir", "victim", 0);
	assertFileMode("victim", 0600);
#endif
}