void ZipTest::testCrcAndSizeAfterDataWithArchive()
{
	std::string testFile = getTestFile("data", "data.zip");
	std::ifstream inp(testFile.c_str(), std::ios::binary);
	assert (inp.good());
	Poco::Zip::ZipArchive zip(inp);
	inp.clear();
	inp.seekg(0);
	Poco::Zip::ZipArchive::FileHeaders::const_iterator it = zip.headerBegin();
	for ( ; it!=zip.headerEnd(); ++it)
	{
		Poco::Zip::ZipInputStream zipis(inp,it->second);
		Poco::Path path(it->second.getFileName());
		if (path.isFile())
		{
			std::ofstream os("test.dat");
			Poco::StreamCopier::copyStream(zipis,os);
		}
	}
}