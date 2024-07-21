static void read_file_into_memory(
    char const* filename,
    PointerHolder<char>& file_buf, size_t& size)
{
    FILE* f = QUtil::safe_fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    size = QUtil::tell(f);
    fseek(f, 0, SEEK_SET);
    file_buf = PointerHolder<char>(true, new char[size]);
    char* buf_p = file_buf.getPointer();
    size_t bytes_read = 0;
    size_t len = 0;
    while ((len = fread(buf_p + bytes_read, 1, size - bytes_read, f)) > 0)
    {
        bytes_read += len;
    }
    if (bytes_read != size)
    {
        if (ferror(f))
        {
            throw std::runtime_error(
                std::string("failure reading file ") + filename +
                " into memory: read " +
                QUtil::uint_to_string(bytes_read) + "; wanted " +
                QUtil::uint_to_string(size));
        }
        else
        {
            throw std::logic_error(
                std::string("premature eof reading file ") + filename +
                " into memory: read " +
                QUtil::uint_to_string(bytes_read) + "; wanted " +
                QUtil::uint_to_string(size));
        }
    }
    fclose(f);
}