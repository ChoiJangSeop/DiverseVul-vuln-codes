Pl_PNGFilter::encodeRow()
{
    // For now, hard-code to using UP filter.
    unsigned char ch = 2;
    getNext()->write(&ch, 1);
    if (this->prev_row)
    {
	for (unsigned int i = 0; i < this->bytes_per_row; ++i)
	{
	    ch = this->cur_row[i] - this->prev_row[i];
	    getNext()->write(&ch, 1);
	}
    }
    else
    {
	getNext()->write(this->cur_row, this->bytes_per_row);
    }
}