QPDF::decryptString(std::string& str, int objid, int generation)
{
    if (objid == 0)
    {
	return;
    }
    bool use_aes = false;
    if (this->m->encp->encryption_V >= 4)
    {
	switch (this->m->encp->cf_string)
	{
	  case e_none:
	    return;

	  case e_aes:
	    use_aes = true;
	    break;

	  case e_aesv3:
	    use_aes = true;
	    break;

	  case e_rc4:
	    break;

	  default:
	    warn(QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			 this->m->last_object_description,
			 this->m->file->getLastOffset(),
			 "unknown encryption filter for strings"
			 " (check /StrF in /Encrypt dictionary);"
			 " strings may be decrypted improperly"));
	    // To avoid repeated warnings, reset cf_string.  Assume
	    // we'd want to use AES if V == 4.
	    this->m->encp->cf_string = e_aes;
            use_aes = true;
	    break;
	}
    }

    std::string key = getKeyForObject(
        this->m->encp, objid, generation, use_aes);
    try
    {
	if (use_aes)
	{
	    QTC::TC("qpdf", "QPDF_encryption aes decode string");
	    Pl_Buffer bufpl("decrypted string");
	    Pl_AES_PDF pl("aes decrypt string", &bufpl, false,
			  QUtil::unsigned_char_pointer(key),
                          key.length());
	    pl.write(QUtil::unsigned_char_pointer(str), str.length());
	    pl.finish();
	    PointerHolder<Buffer> buf = bufpl.getBuffer();
	    str = std::string(reinterpret_cast<char*>(buf->getBuffer()),
                              buf->getSize());
	}
	else
	{
	    QTC::TC("qpdf", "QPDF_encryption rc4 decode string");
	    unsigned int vlen = str.length();
	    // Using PointerHolder guarantees that tmp will
	    // be freed even if rc4.process throws an exception.
	    PointerHolder<char> tmp(true, QUtil::copy_string(str));
	    RC4 rc4(QUtil::unsigned_char_pointer(key), key.length());
	    rc4.process(QUtil::unsigned_char_pointer(tmp.getPointer()), vlen);
	    str = std::string(tmp.getPointer(), vlen);
	}
    }
    catch (QPDFExc&)
    {
	throw;
    }
    catch (std::runtime_error& e)
    {
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      this->m->last_object_description,
		      this->m->file->getLastOffset(),
		      "error decrypting string for object " +
		      QUtil::int_to_string(objid) + " " +
		      QUtil::int_to_string(generation) + ": " + e.what());
    }
}