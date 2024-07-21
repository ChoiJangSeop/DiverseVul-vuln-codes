void pad_short_parameter(std::string& param, unsigned int max_len)
{
    if (param.length() < max_len)
    {
        QTC::TC("qpdf", "QPDF_encryption pad short parameter");
        param.append(max_len - param.length(), '\0');
    }
}