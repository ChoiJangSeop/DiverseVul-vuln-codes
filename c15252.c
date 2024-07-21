MD5::checkFileChecksum(char const* const checksum,
		       char const* filename, int up_to_size)
{
    bool result = false;
    try
    {
	std::string actual_checksum = getFileChecksum(filename, up_to_size);
	result = (checksum == actual_checksum);
    }
    catch (std::runtime_error const&)
    {
	// Ignore -- return false
    }
    return result;
}