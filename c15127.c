Pl_PNGFilter::decodePaeth()
{
    QTC::TC("libtests", "Pl_PNGFilter decodePaeth");
    unsigned char* buffer = this->cur_row + 1;
    unsigned char* above_buffer = this->prev_row + 1;
    unsigned int bpp = this->bytes_per_pixel;

    for (unsigned int i = 0; i < this->bytes_per_row; ++i)
    {
        int left = 0;
        int up = above_buffer[i];
        int upper_left = 0;

        if (i >= bpp)
        {
            left = buffer[i - bpp];
            upper_left = above_buffer[i - bpp];
        }

        buffer[i] += this->PaethPredictor(left, up, upper_left);
    }
}