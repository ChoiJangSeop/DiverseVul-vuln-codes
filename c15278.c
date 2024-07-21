QPDFWriter::copyEncryptionParameters(QPDF& qpdf)
{
    this->m->preserve_encryption = false;
    QPDFObjectHandle trailer = qpdf.getTrailer();
    if (trailer.hasKey("/Encrypt"))
    {
        generateID();
        this->m->id1 =
            trailer.getKey("/ID").getArrayItem(0).getStringValue();
	QPDFObjectHandle encrypt = trailer.getKey("/Encrypt");
	int V = encrypt.getKey("/V").getIntValue();
	int key_len = 5;
	if (V > 1)
	{
	    key_len = encrypt.getKey("/Length").getIntValue() / 8;
	}
	if (encrypt.hasKey("/EncryptMetadata") &&
	    encrypt.getKey("/EncryptMetadata").isBool())
	{
	    this->m->encrypt_metadata =
		encrypt.getKey("/EncryptMetadata").getBoolValue();
	}
        if (V >= 4)
        {
            // When copying encryption parameters, use AES even if the
            // original file did not.  Acrobat doesn't create files
            // with V >= 4 that don't use AES, and the logic of
            // figuring out whether AES is used or not is complicated
            // with /StmF, /StrF, and /EFF all potentially having
            // different values.
            this->m->encrypt_use_aes = true;
        }
	QTC::TC("qpdf", "QPDFWriter copy encrypt metadata",
		this->m->encrypt_metadata ? 0 : 1);
        QTC::TC("qpdf", "QPDFWriter copy use_aes",
                this->m->encrypt_use_aes ? 0 : 1);
        std::string OE;
        std::string UE;
        std::string Perms;
        std::string encryption_key;
        if (V >= 5)
        {
            QTC::TC("qpdf", "QPDFWriter copy V5");
	    OE = encrypt.getKey("/OE").getStringValue();
            UE = encrypt.getKey("/UE").getStringValue();
	    Perms = encrypt.getKey("/Perms").getStringValue();
            encryption_key = qpdf.getEncryptionKey();
        }

	setEncryptionParametersInternal(
	    V,
	    encrypt.getKey("/R").getIntValue(),
    	    key_len,
	    encrypt.getKey("/P").getIntValue(),
	    encrypt.getKey("/O").getStringValue(),
	    encrypt.getKey("/U").getStringValue(),
            OE,
            UE,
            Perms,
	    this->m->id1,		// this->m->id1 == the other file's id1
	    qpdf.getPaddedUserPassword(),
            encryption_key);
    }
}