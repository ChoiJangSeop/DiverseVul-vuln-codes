main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/errors/non_svg_element", test_non_svg_element);

    g_test_add_data_func_full ("/errors/instancing_limit/323-nested-use.svg",
                               "323-nested-use.svg",
                               test_instancing_limit,
                               NULL);

    g_test_add_data_func_full ("/errors/instancing_limit/515-pattern-billion-laughs.svg",
                               "515-pattern-billion-laughs.svg",
                               test_instancing_limit,
                               NULL);


    return g_test_run ();
}