int main(int argc, char *argv[])
{
	int rc;
	struct util_options opts;
	scmp_filter_ctx ctx = NULL;

	rc = util_getopt(argc, argv, &opts);
	if (rc < 0)
		goto out;

	ctx = seccomp_init(SCMP_ACT_KILL);
	if (ctx == NULL)
		return ENOMEM;

	/* note - a "hole" was intentionally left between 64 and 128.
	 * reads of this size should fall through to the default action -
	 * SCMP_ACT_KILL in this test's case.
	 */

	rc = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 1,
			      SCMP_A2(SCMP_CMP_LE, 64));
	if (rc != 0)
		goto out;
	rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(5), SCMP_SYS(read), 1,
			      SCMP_A2(SCMP_CMP_GT, 128));
	if (rc != 0)
		goto out;
	rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(6), SCMP_SYS(read), 1,
			      SCMP_A2(SCMP_CMP_GT, 256));
	if (rc != 0)
		goto out;
	rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(7), SCMP_SYS(read), 1,
			      SCMP_A2(SCMP_CMP_GT, 512));
	if (rc != 0)
		goto out;
	rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(8), SCMP_SYS(read), 1,
			      SCMP_A2(SCMP_CMP_GT, 1024));
	if (rc != 0)
		goto out;
	rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(9), SCMP_SYS(read), 1,
			      SCMP_A2(SCMP_CMP_GT, 2048));
	if (rc != 0)
		goto out;
	rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(10), SCMP_SYS(read), 1,
			      SCMP_A2(SCMP_CMP_GT, 4096));
	if (rc != 0)
		goto out;
	rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(11), SCMP_SYS(read), 1,
			      SCMP_A2(SCMP_CMP_GT, 8192));
	if (rc != 0)
		goto out;
	rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(12), SCMP_SYS(read), 1,
			      SCMP_A2(SCMP_CMP_GT, 16536));
	if (rc != 0)
		goto out;

	rc = util_filter_output(&opts, ctx);
	if (rc)
		goto out;

out:
	seccomp_release(ctx);
	return (rc < 0 ? -rc : rc);
}