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
    int pageno = QUtil::string_to_int(argv[2]);

    try
    {
	QPDF pdf;
	pdf.processFile(filename);
        std::vector<QPDFPageObjectHelper> pages =
            QPDFPageDocumentHelper(pdf).getAllPages();
        if ((pageno < 1) || (static_cast<size_t>(pageno) > pages.size()))
        {
            usage();
        }

        QPDFPageObjectHelper& page = pages.at(pageno-1);
        ParserCallbacks cb;
        page.parsePageContents(&cb);
    }
    catch (std::exception& e)
    {
	std::cerr << whoami << ": " << e.what() << std::endl;
	exit(2);
    }

    return 0;
}