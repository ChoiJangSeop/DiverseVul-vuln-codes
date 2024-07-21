void ZipTest::testDecompressFlat()
{
	std::string testFile = getTestFile("data", "test.zip");
	std::ifstream inp(testFile.c_str(), std::ios::binary);
	assert (inp.good());
	Decompress dec(inp, Poco::Path(), true);
	dec.EError += Poco::Delegate<ZipTest, std::pair<const Poco::Zip::ZipLocalFileHeader, const std::string> >(this, &ZipTest::onDecompressError);
	dec.decompressAllFiles();
	dec.EError -= Poco::Delegate<ZipTest, std::pair<const Poco::Zip::ZipLocalFileHeader, const std::string> >(this, &ZipTest::onDecompressError);
	assert (_errCnt == 0);
	assert (!dec.mapping().empty());
}