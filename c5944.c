QPDF::parse_xrefEntry(std::string const& line,
                      qpdf_offset_t& f1, int& f2, char& type)
{
    // is_space and is_digit both return false on '\0', so this will
    // not overrun the null-terminated buffer.
    char const* p = line.c_str();

    // Skip zero or more spaces. There aren't supposed to be any.
    bool invalid = false;
    while (QUtil::is_space(*p))
    {
        ++p;
        QTC::TC("qpdf", "QPDF ignore first space in xref entry");
        invalid = true;
    }
    // Require digit
    if (! QUtil::is_digit(*p))
    {
        return false;
    }
    // Gather digits
    std::string f1_str;
    while (QUtil::is_digit(*p))
    {
        f1_str.append(1, *p++);
    }
    // Require space
    if (! QUtil::is_space(*p))
    {
        return false;
    }
    if (QUtil::is_space(*(p+1)))
    {
        QTC::TC("qpdf", "QPDF ignore first extra space in xref entry");
        invalid = true;
    }
    // Skip spaces
    while (QUtil::is_space(*p))
    {
        ++p;
    }
    // Require digit
    if (! QUtil::is_digit(*p))
    {
        return false;
    }
    // Gather digits
    std::string f2_str;
    while (QUtil::is_digit(*p))
    {
        f2_str.append(1, *p++);
    }
    // Require space
    if (! QUtil::is_space(*p))
    {
        return false;
    }
    if (QUtil::is_space(*(p+1)))
    {
        QTC::TC("qpdf", "QPDF ignore second extra space in xref entry");
        invalid = true;
    }
    // Skip spaces
    while (QUtil::is_space(*p))
    {
        ++p;
    }
    if ((*p == 'f') || (*p == 'n'))
    {
        type = *p;
    }
    else
    {
        return false;
    }
    if ((f1_str.length() != 10) || (f2_str.length() != 5))
    {
        QTC::TC("qpdf", "QPDF ignore length error xref entry");
        invalid = true;
    }

    if (invalid)
    {
        warn(QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                     "xref table",
                     this->m->file->getLastOffset(),
                     "accepting invalid xref table entry"));
    }

    f1 = QUtil::string_to_ll(f1_str.c_str());
    f2 = atoi(f2_str.c_str());

    return true;
}