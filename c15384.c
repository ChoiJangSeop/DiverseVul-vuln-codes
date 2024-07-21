void TestJlCompress::extractDir()
{
    QFETCH(QString, zipName);
    QFETCH(QStringList, fileNames);
    QDir curDir;
    if (!curDir.mkpath("jlext/jldir")) {
        QFAIL("Couldn't mkpath jlext/jldir");
    }
    if (!createTestFiles(fileNames)) {
        QFAIL("Couldn't create test files");
    }
    if (!createTestArchive(zipName, fileNames)) {
        QFAIL("Couldn't create test archive");
    }
    QStringList extracted;
    QCOMPARE((extracted = JlCompress::extractDir(zipName, "jlext/jldir"))
        .count(), fileNames.count());
    foreach (QString fileName, fileNames) {
        QString fullName = "jlext/jldir/" + fileName;
        QFileInfo fileInfo(fullName);
        QFileInfo extInfo("tmp/" + fileName);
        if (!fileInfo.isDir())
            QCOMPARE(fileInfo.size(), extInfo.size());
        QCOMPARE(fileInfo.permissions(), extInfo.permissions());
        curDir.remove(fullName);
        curDir.rmpath(fileInfo.dir().path());
        QString absolutePath = fileInfo.absoluteFilePath();
        if (fileInfo.isDir() && !absolutePath.endsWith('/'))
	    absolutePath += '/';
        QVERIFY(extracted.contains(absolutePath));
    }
    // now test the QIODevice* overload
    QFile zipFile(zipName);
    QVERIFY(zipFile.open(QIODevice::ReadOnly));
    QCOMPARE((extracted = JlCompress::extractDir(&zipFile, "jlext/jldir"))
        .count(), fileNames.count());
    foreach (QString fileName, fileNames) {
        QString fullName = "jlext/jldir/" + fileName;
        QFileInfo fileInfo(fullName);
        QFileInfo extInfo("tmp/" + fileName);
        if (!fileInfo.isDir())
            QCOMPARE(fileInfo.size(), extInfo.size());
        QCOMPARE(fileInfo.permissions(), extInfo.permissions());
        curDir.remove(fullName);
        curDir.rmpath(fileInfo.dir().path());
        QString absolutePath = fileInfo.absoluteFilePath();
        if (fileInfo.isDir() && !absolutePath.endsWith('/'))
        absolutePath += '/';
        QVERIFY(extracted.contains(absolutePath));
    }
    zipFile.close();
    curDir.rmpath("jlext/jldir");
    removeTestFiles(fileNames);
    curDir.remove(zipName);
}