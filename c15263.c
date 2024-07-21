QPDF::adjusted_offset(int offset)
{
    // All offsets >= H_offset have to be increased by H_length
    // since all hint table location values disregard the hint table
    // itself.
    if (offset >= this->m->linp.H_offset)
    {
	return offset + this->m->linp.H_length;
    }
    return offset;
}