int main(void) {
    char *output = NULL;
    CuSuite* suite = CuSuiteNew();

    abs_top_srcdir = getenv("abs_top_srcdir");
    if (abs_top_srcdir == NULL)
        die("env var abs_top_srcdir must be set");

    abs_top_builddir = getenv("abs_top_builddir");
    if (abs_top_builddir == NULL)
        die("env var abs_top_builddir must be set");

    if (asprintf(&src_root, "%s/tests/root", abs_top_srcdir) < 0) {
        die("failed to set src_root");
    }

    CuSuiteSetup(suite, setup, teardown);

    SUITE_ADD_TEST(suite, testSaveNewFile);
    SUITE_ADD_TEST(suite, testNonExistentLens);
    SUITE_ADD_TEST(suite, testMultipleXfm);
    SUITE_ADD_TEST(suite, testMtime);
    SUITE_ADD_TEST(suite, testRelPath);
    SUITE_ADD_TEST(suite, testDoubleSlashPath);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, &output);
    CuSuiteDetails(suite, &output);
    printf("%s\n", output);
    free(output);
    return suite->failCount;
}