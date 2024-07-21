int main(int argc, char* argv[])
{
    whoami = QUtil::getWhoami(argv[0]);

    // For libtool's sake....
    if (strncmp(whoami, "lt-", 3) == 0)
    {
	whoami += 3;
    }

    if (argc != 3)
    {
	usage();
    }
    char const* filename = argv[1];
    int pageno = atoi(argv[2]);

    try
    {
	QPDF pdf;
	pdf.processFile(filename);
        std::vector<QPDFObjectHandle> pages = pdf.getAllPages();
        if ((pageno < 1) || (static_cast<size_t>(pageno) > pages.size()))
        {
            usage();
        }

        QPDFObjectHandle page = pages.at(pageno-1);
        QPDFObjectHandle contents = page.getKey("/Contents");
        ParserCallbacks cb;
        QPDFObjectHandle::parseContentStream(contents, &cb);
    }
    catch (std::exception& e)
    {
	std::cerr << whoami << ": " << e.what() << std::endl;
	exit(2);
    }

    return 0;
}