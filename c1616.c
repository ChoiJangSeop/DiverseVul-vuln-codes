static void *my_malloc(size_t size)
{
    malloc_called += 1;
    return malloc(size);
}