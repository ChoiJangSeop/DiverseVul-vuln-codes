MD5::getFileChecksum(char const* filename, int up_to_size)
{
    MD5 m;
    m.encodeFile(filename, up_to_size);
    return m.unparse();
}