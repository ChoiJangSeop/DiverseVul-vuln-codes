QPDF::trim_user_password(std::string& user_password)
{
    // Although unnecessary, this routine trims the padding string
    // from the end of a user password.  Its only purpose is for
    // recovery of user passwords which is done in the test suite.
    char const* cstr = user_password.c_str();
    size_t len = user_password.length();
    if (len < key_bytes)
    {
	return;
    }

    char const* p1 = cstr;
    char const* p2 = 0;
    while ((p2 = strchr(p1, '\x28')) != 0)
    {
	if (memcmp(p2, padding_string, len - (p2 - cstr)) == 0)
	{
	    user_password = user_password.substr(0, p2 - cstr);
	    return;
	}
        else
        {
            QTC::TC("qpdf", "QPDF_encryption skip 0x28");
            p1 = p2 + 1;
        }
    }
}