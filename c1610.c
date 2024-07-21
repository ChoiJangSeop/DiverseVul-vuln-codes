static void my_free(void *ptr)
{
    free_called += 1;
    free(ptr);
}