static void test_simple()
{
    json_set_alloc_funcs(my_malloc, my_free);
    create_and_free_complex_object();

    if(malloc_called != 20 || free_called != 20)
        fail("Custom allocation failed");
}