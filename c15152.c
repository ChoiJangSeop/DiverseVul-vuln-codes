static void process(char const* whoami,
                    char const* infile,
                    std::string outprefix)
{
    QPDF inpdf;
    inpdf.processFile(infile);
    std::vector<QPDFPageObjectHelper> pages =
        QPDFPageDocumentHelper(inpdf).getAllPages();
    int pageno_len = QUtil::int_to_string(pages.size()).length();
    int pageno = 0;
    for (std::vector<QPDFPageObjectHelper>::iterator iter = pages.begin();
         iter != pages.end(); ++iter)
    {
        QPDFPageObjectHelper& page(*iter);
        std::string outfile =
            outprefix + QUtil::int_to_string(++pageno, pageno_len) + ".pdf";
        QPDF outpdf;
        outpdf.emptyPDF();
        QPDFPageDocumentHelper(outpdf).addPage(page, false);
        QPDFWriter outpdfw(outpdf, outfile.c_str());
	if (static_id)
	{
	    // For the test suite, uncompress streams and use static
	    // IDs.
	    outpdfw.setStaticID(true); // for testing only
	    outpdfw.setStreamDataMode(qpdf_s_uncompress);
	}
        outpdfw.write();
    }
}