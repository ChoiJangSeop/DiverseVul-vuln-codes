QPDF::parse_xrefFirst(std::string const& line,
                      int& obj, int& num, int& bytes)
{
    // is_space and is_digit both return false on '\0', so this will
    // not overrun the null-terminated buffer.
    char const* p = line.c_str();
    char const* start = line.c_str();

    // Skip zero or more spaces
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
    std::string obj_str;
    while (QUtil::is_digit(*p))
    {
        obj_str.append(1, *p++);
    }
    // Require space
    if (! QUtil::is_space(*p))
    {
        return false;
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
    std::string num_str;
    while (QUtil::is_digit(*p))
    {
        num_str.append(1, *p++);
    }
    // Skip any space including line terminators
    while (QUtil::is_space(*p))
    {
        ++p;
    }
    bytes = p - start;
    obj = atoi(obj_str.c_str());
    num = atoi(num_str.c_str());
    return true;
}