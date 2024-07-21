void ZipTest::verifyDataFile(const std::string& path, Poco::UInt64 size)
{
	std::ifstream in(path.c_str(), std::ios::binary);
	assert( ! in.fail() );
	Poco::Buffer<char> buffer1(MB);
	Poco::Buffer<char> buffer2(MB);
	for (int i = 0; size != 0; i++)
	{
		std::memset(buffer1.begin(), i, buffer1.size());
		std::memset(buffer2.begin(), 0, buffer2.size());
		Poco::UInt64 bytesToRead = std::min(size, static_cast<Poco::UInt64>(buffer2.size()));
		in.read(buffer2.begin(), bytesToRead);
		assert(!in.fail() );
		assert(std::memcmp(buffer1.begin(), buffer2.begin(), static_cast<std::size_t>(bytesToRead)) == 0);
		size -= bytesToRead;
	}
	char c;
	in.read(&c, 1);
	assert ( in.eof() );
}