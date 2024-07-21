void CompressTest::testSetZipComment()
{
	std::string comment("Testing...123...");
	std::ofstream out("comment.zip", std::ios::binary);
	Poco::Path theFile(ZipTest::getTestFile("data", "test.zip"));
	Compress c(out, true);
	c.addFile(theFile, theFile.getFileName());
	c.setZipComment(comment);
	ZipArchive a(c.close());
	assert(a.getZipComment() == comment);
}