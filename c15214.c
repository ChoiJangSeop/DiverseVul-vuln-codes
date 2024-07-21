MD5::checkDataChecksum(char const* const checksum,
		       char const* buf, int len)
{
    std::string actual_checksum = getDataChecksum(buf, len);
    return (checksum == actual_checksum);
}