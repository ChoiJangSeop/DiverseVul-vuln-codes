FileInputSource::read(char* buffer, size_t length)
{
    this->last_offset = this->tell();
    size_t len = fread(buffer, 1, length, this->file);
    if (len == 0)
    {
        if (ferror(this->file))
        {
            throw QPDFExc(qpdf_e_system,
                          this->filename, "",
                          this->last_offset,
                          std::string("read ") +
                          QUtil::int_to_string(length) + " bytes");
        }
        else if (length > 0)
        {
            this->seek(0, SEEK_END);
            this->last_offset = this->tell();
        }
    }
    return len;
}