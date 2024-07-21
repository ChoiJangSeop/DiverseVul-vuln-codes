QPDFPageData::QPDFPageData(std::string const& filename,
                           QPDF* qpdf,
                           char const* range) :
    filename(filename),
    qpdf(qpdf),
    orig_pages(qpdf->getAllPages())
{
    try
    {
        this->selected_pages =
            QUtil::parse_numrange(range, this->orig_pages.size());
    }
    catch (std::runtime_error& e)
    {
        usageExit("parsing numeric range for " + filename + ": " + e.what());
    }
}