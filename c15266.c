static void handle_rotations(QPDF& pdf, Options& o)
{
    QPDFPageDocumentHelper dh(pdf);
    std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
    int npages = static_cast<int>(pages.size());
    for (std::map<std::string, RotationSpec>::iterator iter =
             o.rotations.begin();
         iter != o.rotations.end(); ++iter)
    {
        std::string const& range = (*iter).first;
        RotationSpec const& rspec = (*iter).second;
        // range has been previously validated
        std::vector<int> to_rotate =
            QUtil::parse_numrange(range.c_str(), npages);
        for (std::vector<int>::iterator i2 = to_rotate.begin();
             i2 != to_rotate.end(); ++i2)
        {
            int pageno = *i2 - 1;
            if ((pageno >= 0) && (pageno < npages))
            {
                pages.at(pageno).rotatePage(rspec.angle, rspec.relative);
            }
        }
    }
}