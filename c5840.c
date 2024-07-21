void CompressTest::testDirectory()
{
	std::ofstream out("pocobin.zip", std::ios::binary);
	Poco::File aFile("some/");
	if (aFile.exists())
		aFile.remove(true);
	Poco::File aDir("some/recursive/dir/");
	aDir.createDirectories();
	Poco::File aDir2("some/other/recursive/dir/");
	aDir2.createDirectories();
	Poco::File aF("some/recursive/dir/test.file");
	aF.createFile();
	Poco::FileOutputStream fos(aF.path());
	fos << "just some test data";
	fos.close();

	Poco::Path theFile(aFile.path());
	theFile.makeDirectory();
	Compress c(out, true);
	c.addRecursive(theFile, ZipCommon::CL_MAXIMUM, false, theFile);
	ZipArchive a(c.close());
}