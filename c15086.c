int_to_string_base_internal(T num, int base, int length)
{
    // Backward compatibility -- int_to_string, which calls this
    // function, used to use sprintf with %0*d, so we interpret length
    // such that a negative value appends spaces and a positive value
    // prepends zeroes.
    if (! ((base == 8) || (base == 10) || (base == 16)))
    {
        throw std::logic_error(
            "int_to_string_base called with unsupported base");
    }
    std::ostringstream buf;
    buf << std::setbase(base) << std::nouppercase << num;
    std::string result;
    if ((length > 0) &&
        (buf.str().length() < QIntC::to_size(length)))
    {
	result.append(length - buf.str().length(), '0');
    }
    result += buf.str();
    if ((length < 0) && (buf.str().length() < QIntC::to_size(-length)))
    {
	result.append(-length - buf.str().length(), ' ');
    }
    return result;
}