void TestJlCompress::extractDir_data()
{
    QTest::addColumn<QString>("zipName");
    QTest::addColumn<QStringList>("fileNames");
    QTest::newRow("simple") << "jlextdir.zip" << (
            QStringList() << "test0.txt" << "testdir1/test1.txt"
            << "testdir2/test2.txt" << "testdir2/subdir/test2sub.txt");
    QTest::newRow("separate dir") << "sepdir.zip" << (
            QStringList() << "laj/" << "laj/lajfile.txt");
}