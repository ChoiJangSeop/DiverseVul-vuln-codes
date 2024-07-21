Pl_PNGFilter::decodeAverage()
{
    QTC::TC("libtests", "Pl_PNGFilter decodeAverage");
    unsigned char* buffer = this->cur_row + 1;
    unsigned char* above_buffer = this->prev_row + 1;
    unsigned int bpp = this->bytes_per_pixel;

    for (unsigned int i = 0; i < this->bytes_per_row; ++i)
    {
        int left = 0;
        int up = 0;

        if (i >= bpp)
        {
            left = buffer[i - bpp];
        }

        up = above_buffer[i];
        buffer[i] += (left+up) / 2;
    }
}