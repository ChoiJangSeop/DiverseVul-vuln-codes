int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    /* try to create a surface with no issues, should succeed */
    g_test_add_func("/server/qxl-parsing-no-issues", test_no_issues);

    /* try to create a surface with a stride too small to fit
     * the entire width.
     * This can be used to cause buffer overflows so refuse it.
     */
    g_test_add_func("/server/qxl-parsing/stride-too-small", test_stride_too_small);

    /* try to create a surface quite large.
     * The sizes (width and height) were chosen so the multiplication
     * using 32 bit values gives a very small value.
     * These kind of values should be refused as they will cause
     * overflows. Also the total memory for the card is not enough to
     * hold the surface so surely can't be accepted.
     */
    g_test_add_func("/server/qxl-parsing/too-big-image", test_too_big_image);

    /* test base cursor with no problems */
    g_test_add_func("/server/qxl-parsing/base-cursor-command", test_cursor_command);

    /* a circular list of empty chunks should not be a problems */
    g_test_add_func("/server/qxl-parsing/circular-empty-chunks", test_circular_empty_chunks);

    /* a circular list of small chunks should not be a problems */
    g_test_add_func("/server/qxl-parsing/circular-small-chunks", test_circular_small_chunks);

    return g_test_run();
}