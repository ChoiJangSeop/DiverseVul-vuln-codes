QEMUCursor *cursor_alloc(int width, int height)
{
    QEMUCursor *c;
    int datasize = width * height * sizeof(uint32_t);

    c = g_malloc0(sizeof(QEMUCursor) + datasize);
    c->width  = width;
    c->height = height;
    c->refcount = 1;
    return c;
}