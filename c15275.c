InputSource::findLast(char const* start_chars,
                      qpdf_offset_t offset, size_t len,
                      Finder& finder)
{
    bool found = false;
    qpdf_offset_t after_found_offset = 0;
    qpdf_offset_t cur_offset = offset;
    size_t cur_len = len;
    while (this->findFirst(start_chars, cur_offset, cur_len, finder))
    {
        if (found)
        {
            QTC::TC("libtests", "InputSource findLast found more than one");
        }
        else
        {
            found = true;
        }
        after_found_offset = this->tell();
        cur_offset = after_found_offset;
        cur_len = len - (cur_offset - offset);
    }
    if (found)
    {
        this->seek(after_found_offset, SEEK_SET);
    }
    return found;
}