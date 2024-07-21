QUtil::hex_decode(std::string const& input)
{
    std::string result;
    size_t pos = 0;
    for (std::string::const_iterator p = input.begin(); p != input.end(); ++p)
    {
        char ch = *p;
        bool skip = false;
        if ((*p >= 'A') && (*p <= 'F'))
        {
            ch -= 'A';
            ch += 10;
        }
        else if ((*p >= 'a') && (*p <= 'f'))
        {
            ch -= 'a';
            ch += 10;
        }
        else if ((*p >= '0') && (*p <= '9'))
        {
            ch -= '0';
        }
        else
        {
            skip = true;
        }
        if (! skip)
        {
            if (pos == 0)
            {
                result.push_back(ch << 4);
                pos = 1;
            }
            else
            {
                result[result.length()-1] += ch;
                pos = 0;
            }
        }
    }
    return result;
}