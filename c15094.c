JSON::encode_string(std::string const& str)
{
    std::string result;
    size_t len = str.length();
    for (size_t i = 0; i < len; ++i)
    {
        unsigned char ch = static_cast<unsigned char>(str.at(i));
        switch (ch)
        {
          case '\\':
            result += "\\\\";
            break;
          case '\"':
            result += "\\\"";
            break;
          case '\b':
            result += "\\b";
            break;
          case '\n':
            result += "\\n";
            break;
          case '\r':
            result += "\\r";
            break;
          case '\t':
            result += "\\t";
            break;
          default:
            if (ch < 32)
            {
                result += "\\u" + QUtil::int_to_string_base(ch, 16, 4);
            }
            else
            {
                result.append(1, ch);
            }
        }
    }
    return result;
}