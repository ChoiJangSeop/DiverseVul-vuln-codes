Pl_PNGFilter::decodeUp()
{
    QTC::TC("libtests", "Pl_PNGFilter decodeUp");
    unsigned char* buffer = this->cur_row + 1;
    unsigned char* above_buffer = this->prev_row + 1;

    for (unsigned int i = 0; i < this->bytes_per_row; ++i)
    {
        unsigned char up = above_buffer[i];
        buffer[i] += up;
    }
}