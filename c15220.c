static void check_pdf(char const* filename)
{
    QPDF pdf;
    pdf.processFile(filename);
    std::vector<QPDFObjectHandle> const& pages = pdf.getAllPages();
    assert(pages.size() == static_cast<size_t>(npages));
    for (int i = 0; i < npages; ++i)
    {
        int pageno = i + 1;
        std::cout << "page " << pageno << " of " << npages << std::endl;
        check_page_contents(pageno, pages.at(i));
        check_image(pageno, pages.at(i));
    }
}