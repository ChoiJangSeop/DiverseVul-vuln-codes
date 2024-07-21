static void png_write_data(png_structp png_ptr, png_bytep data,
                           png_size_t length)
{
    VncState *vs = png_get_io_ptr(png_ptr);

    buffer_reserve(&vs->tight.png, vs->tight.png.offset + length);
    memcpy(vs->tight.png.buffer + vs->tight.png.offset, data, length);

    vs->tight.png.offset += length;
}