QPDFWriter::parseVersion(std::string const& version,
			 int& major, int& minor) const
{
    major = atoi(version.c_str());
    minor = 0;
    size_t p = version.find('.');
    if ((p != std::string::npos) && (version.length() > p))
    {
	minor = atoi(version.substr(p + 1).c_str());
    }
    std::string tmp = QUtil::int_to_string(major) + "." +
	QUtil::int_to_string(minor);
    if (tmp != version)
    {
	throw std::logic_error(
	    "INTERNAL ERROR: QPDFWriter::parseVersion"
	    " called with invalid version number " + version);
    }
}