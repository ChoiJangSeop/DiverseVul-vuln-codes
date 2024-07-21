QPDFWriter::disableIncompatibleEncryption(int major, int minor,
                                          int extension_level)
{
    if (! this->m->encrypted)
    {
	return;
    }

    bool disable = false;
    if (compareVersions(major, minor, 1, 3) < 0)
    {
	disable = true;
    }
    else
    {
	int V = atoi(this->m->encryption_dictionary["/V"].c_str());
	int R = atoi(this->m->encryption_dictionary["/R"].c_str());
	if (compareVersions(major, minor, 1, 4) < 0)
	{
	    if ((V > 1) || (R > 2))
	    {
		disable = true;
	    }
	}
	else if (compareVersions(major, minor, 1, 5) < 0)
	{
	    if ((V > 2) || (R > 3))
	    {
		disable = true;
	    }
	}
	else if (compareVersions(major, minor, 1, 6) < 0)
	{
	    if (this->m->encrypt_use_aes)
	    {
		disable = true;
	    }
	}
        else if ((compareVersions(major, minor, 1, 7) < 0) ||
                 ((compareVersions(major, minor, 1, 7) == 0) &&
                  extension_level < 3))
        {
            if ((V >= 5) || (R >= 5))
            {
                disable = true;
            }
        }
    }
    if (disable)
    {
	QTC::TC("qpdf", "QPDFWriter forced version disabled encryption");
	this->m->encrypted = false;
    }
}