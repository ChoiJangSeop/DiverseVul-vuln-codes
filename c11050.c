ga_init2(garray_T *gap, int itemsize, int growsize)
{
    ga_init(gap);
    gap->ga_itemsize = itemsize;
    gap->ga_growsize = growsize;
}