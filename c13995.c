static void setup(CuTest *tc) {
    char *lensdir;

    if (asprintf(&root, "%s/build/test-save/%s",
                 abs_top_builddir, tc->name) < 0) {
        CuFail(tc, "failed to set root");
    }

    if (asprintf(&lensdir, "%s/lenses", abs_top_srcdir) < 0)
        CuFail(tc, "asprintf lensdir failed");

    run(tc, "test -d %s && chmod -R u+w %s || :", root, root);
    run(tc, "rm -rf %s", root);
    run(tc, "mkdir -p %s", root);
    run(tc, "cp -pr %s/* %s", src_root, root);
    run(tc, "chmod -R u+w %s", root);

    aug = aug_init(root, lensdir, AUG_NO_STDINC);
    CuAssertPtrNotNull(tc, aug);
}