int main(void) {
    char *output = NULL;
    CuSuite* suite = CuSuiteNew();
    CuSuiteSetup(suite, NULL, NULL);
    SUITE_ADD_TEST(suite, testDefault);
    SUITE_ADD_TEST(suite, testNoLoad);
    SUITE_ADD_TEST(suite, testNoAutoload);
    SUITE_ADD_TEST(suite, testInvalidLens);
    SUITE_ADD_TEST(suite, testLoadSave);
    SUITE_ADD_TEST(suite, testLoadDefined);
    SUITE_ADD_TEST(suite, testDefvarExpr);
    SUITE_ADD_TEST(suite, testReloadChanged);
    SUITE_ADD_TEST(suite, testReloadDirty);
    SUITE_ADD_TEST(suite, testReloadDeleted);
    SUITE_ADD_TEST(suite, testReloadDeletedMeta);
    SUITE_ADD_TEST(suite, testReloadExternalMod);
    SUITE_ADD_TEST(suite, testReloadAfterSaveNewfile);
    SUITE_ADD_TEST(suite, testParseErrorReported);
    SUITE_ADD_TEST(suite, testLoadExclWithRoot);
    SUITE_ADD_TEST(suite, testLoadTrailingExcl);

    abs_top_srcdir = getenv("abs_top_srcdir");
    if (abs_top_srcdir == NULL)
        die("env var abs_top_srcdir must be set");

    abs_top_builddir = getenv("abs_top_builddir");
    if (abs_top_builddir == NULL)
        die("env var abs_top_builddir must be set");

    if (asprintf(&root, "%s/tests/root", abs_top_srcdir) < 0) {
        die("failed to set root");
    }

    if (asprintf(&loadpath, "%s/lenses", abs_top_srcdir) < 0) {
        die("failed to set loadpath");
    }

    CuSuiteRun(suite);
    CuSuiteSummary(suite, &output);
    CuSuiteDetails(suite, &output);
    printf("%s\n", output);
    free(output);
    return suite->failCount;
}