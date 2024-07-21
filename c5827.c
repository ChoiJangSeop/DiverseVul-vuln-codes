CppUnit::Test* ZipTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("ZipTest");

	CppUnit_addTest(pSuite, ZipTest, testSkipSingleFile);
	CppUnit_addTest(pSuite, ZipTest, testDecompressSingleFile);
	CppUnit_addTest(pSuite, ZipTest, testDecompressSingleFileInDir);
	CppUnit_addTest(pSuite, ZipTest, testDecompress);
	CppUnit_addTest(pSuite, ZipTest, testDecompressFlat);
	CppUnit_addTest(pSuite, ZipTest, testCrcAndSizeAfterData);
	CppUnit_addTest(pSuite, ZipTest, testCrcAndSizeAfterDataWithArchive);
	CppUnit_addTest(pSuite, ZipTest, testDecompressZip64);

	return pSuite;
}