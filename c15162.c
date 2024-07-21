Pl_PNGFilter::decodeSub()
{
    QTC::TC("libtests", "Pl_PNGFilter decodeSub");
    unsigned char* buffer = this->cur_row + 1;
    unsigned int bpp = this->bytes_per_pixel;

    for (unsigned int i = 0; i < this->bytes_per_row; ++i)
    {
        unsigned char left = 0;

        if (i >= bpp)
        {
            left = buffer[i - bpp];
        }

        buffer[i] += left;
    }
}