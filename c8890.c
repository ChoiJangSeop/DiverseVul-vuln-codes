void test_path_dotgit__dotgit_modules_symlink(void)
{
	cl_assert_equal_b(true, git_path_isvalid(NULL, ".gitmodules", 0, GIT_PATH_REJECT_DOT_GIT_HFS|GIT_PATH_REJECT_DOT_GIT_NTFS));
	cl_assert_equal_b(false, git_path_isvalid(NULL, ".gitmodules", S_IFLNK, GIT_PATH_REJECT_DOT_GIT_HFS));
	cl_assert_equal_b(false, git_path_isvalid(NULL, ".gitmodules", S_IFLNK, GIT_PATH_REJECT_DOT_GIT_NTFS));
}