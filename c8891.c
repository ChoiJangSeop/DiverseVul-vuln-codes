void test_checkout_nasty__git_tilde1(void)
{
#ifdef GIT_WIN32
	test_checkout_fails("refs/heads/git_tilde1", ".git/foobar");
#endif
}