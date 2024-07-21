static void validate_under_overlay(QPDF& pdf, UnderOverlay* uo, Options& o)
{
    if (0 == uo->filename)
    {
        return;
    }
    QPDFPageDocumentHelper main_pdh(pdf);
    int main_npages = static_cast<int>(main_pdh.getAllPages().size());
    uo->pdf = process_file(uo->filename, uo->password, o);
    QPDFPageDocumentHelper uo_pdh(*(uo->pdf));
    int uo_npages = static_cast<int>(uo_pdh.getAllPages().size());
    try
    {
        uo->to_pagenos = QUtil::parse_numrange(uo->to_nr, main_npages);
    }
    catch (std::runtime_error& e)
    {
        usageExit("parsing numeric range for " + uo->which +
                  " \"to\" pages: " + e.what());
    }
    try
    {
        if (0 == strlen(uo->from_nr))
        {
            QTC::TC("qpdf", "qpdf from_nr from repeat_nr");
            uo->from_nr = uo->repeat_nr;
        }
        uo->from_pagenos = QUtil::parse_numrange(uo->from_nr, uo_npages);
        if (strlen(uo->repeat_nr))
        {
            uo->repeat_pagenos =
                QUtil::parse_numrange(uo->repeat_nr, uo_npages);
        }
    }
    catch (std::runtime_error& e)
    {
        usageExit("parsing numeric range for " + uo->which + " file " +
                  uo->filename + ": " + e.what());
    }
}