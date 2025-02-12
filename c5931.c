QPDF::initializeEncryption()
{
    if (this->m->encryption_initialized)
    {
	return;
    }
    this->m->encryption_initialized = true;

    // After we initialize encryption parameters, we must used stored
    // key information and never look at /Encrypt again.  Otherwise,
    // things could go wrong if someone mutates the encryption
    // dictionary.

    if (! this->m->trailer.hasKey("/Encrypt"))
    {
	return;
    }

    // Go ahead and set this->m->encrypted here.  That way, isEncrypted
    // will return true even if there were errors reading the
    // encryption dictionary.
    this->m->encrypted = true;

    std::string id1;
    QPDFObjectHandle id_obj = this->m->trailer.getKey("/ID");
    if ((id_obj.isArray() &&
         (id_obj.getArrayNItems() == 2) &&
         id_obj.getArrayItem(0).isString()))
    {
        id1 = id_obj.getArrayItem(0).getStringValue();
    }
    else
    {
        // Treating a missing ID as the empty string enables qpdf to
        // decrypt some invalid encrypted files with no /ID that
        // poppler can read but Adobe Reader can't.
	warn(QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                     "trailer", this->m->file->getLastOffset(),
                     "invalid /ID in trailer dictionary"));
    }

    QPDFObjectHandle encryption_dict = this->m->trailer.getKey("/Encrypt");
    if (! encryption_dict.isDictionary())
    {
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      this->m->last_object_description,
		      this->m->file->getLastOffset(),
		      "/Encrypt in trailer dictionary is not a dictionary");
    }

    if (! (encryption_dict.getKey("/Filter").isName() &&
	   (encryption_dict.getKey("/Filter").getName() == "/Standard")))
    {
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      "encryption dictionary", this->m->file->getLastOffset(),
		      "unsupported encryption filter");
    }
    if (! encryption_dict.getKey("/SubFilter").isNull())
    {
	warn(QPDFExc(qpdf_e_unsupported, this->m->file->getName(),
		     "encryption dictionary", this->m->file->getLastOffset(),
		     "file uses encryption SubFilters,"
		     " which qpdf does not support"));
    }

    if (! (encryption_dict.getKey("/V").isInteger() &&
	   encryption_dict.getKey("/R").isInteger() &&
	   encryption_dict.getKey("/O").isString() &&
	   encryption_dict.getKey("/U").isString() &&
	   encryption_dict.getKey("/P").isInteger()))
    {
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      "encryption dictionary", this->m->file->getLastOffset(),
		      "some encryption dictionary parameters are missing "
		      "or the wrong type");
    }

    int V = encryption_dict.getKey("/V").getIntValue();
    int R = encryption_dict.getKey("/R").getIntValue();
    std::string O = encryption_dict.getKey("/O").getStringValue();
    std::string U = encryption_dict.getKey("/U").getStringValue();
    unsigned int P = encryption_dict.getKey("/P").getIntValue();

    // If supporting new encryption R/V values, remember to update
    // error message inside this if statement.
    if (! (((R >= 2) && (R <= 6)) &&
	   ((V == 1) || (V == 2) || (V == 4) || (V == 5))))
    {
	throw QPDFExc(qpdf_e_unsupported, this->m->file->getName(),
		      "encryption dictionary", this->m->file->getLastOffset(),
		      "Unsupported /R or /V in encryption dictionary; R = " +
                      QUtil::int_to_string(R) + " (max 6), V = " +
                      QUtil::int_to_string(V) + " (max 5)");
    }

    this->m->encryption_V = V;
    this->m->encryption_R = R;

    // OE, UE, and Perms are only present if V >= 5.
    std::string OE;
    std::string UE;
    std::string Perms;

    if (V < 5)
    {
        pad_short_parameter(O, key_bytes);
        pad_short_parameter(U, key_bytes);
        if (! ((O.length() == key_bytes) && (U.length() == key_bytes)))
        {
            throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                          "encryption dictionary",
                          this->m->file->getLastOffset(),
                          "incorrect length for /O and/or /U in "
                          "encryption dictionary");
        }
    }
    else
    {
        if (! (encryption_dict.getKey("/OE").isString() &&
               encryption_dict.getKey("/UE").isString() &&
               encryption_dict.getKey("/Perms").isString()))
        {
            throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                          "encryption dictionary",
                          this->m->file->getLastOffset(),
                          "some V=5 encryption dictionary parameters are "
                          "missing or the wrong type");
        }
        OE = encryption_dict.getKey("/OE").getStringValue();
        UE = encryption_dict.getKey("/UE").getStringValue();
        Perms = encryption_dict.getKey("/Perms").getStringValue();

        pad_short_parameter(O, OU_key_bytes_V5);
        pad_short_parameter(U, OU_key_bytes_V5);
        pad_short_parameter(OE, OUE_key_bytes_V5);
        pad_short_parameter(UE, OUE_key_bytes_V5);
        pad_short_parameter(Perms, Perms_key_bytes_V5);
        if ((O.length() < OU_key_bytes_V5) ||
            (U.length() < OU_key_bytes_V5) ||
            (OE.length() < OUE_key_bytes_V5) ||
            (UE.length() < OUE_key_bytes_V5) ||
            (Perms.length() < Perms_key_bytes_V5))
        {
            throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                          "encryption dictionary",
                          this->m->file->getLastOffset(),
                          "incorrect length for some of"
                          " /O, /U, /OE, /UE, or /Perms in"
                          " encryption dictionary");
        }
    }

    int Length = 40;
    if (encryption_dict.getKey("/Length").isInteger())
    {
	Length = encryption_dict.getKey("/Length").getIntValue();
	if ((Length % 8) || (Length < 40) || (Length > 256))
	{
	    throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			  "encryption dictionary",
                          this->m->file->getLastOffset(),
			  "invalid /Length value in encryption dictionary");
	}
    }

    this->m->encrypt_metadata = true;
    if ((V >= 4) && (encryption_dict.getKey("/EncryptMetadata").isBool()))
    {
	this->m->encrypt_metadata =
	    encryption_dict.getKey("/EncryptMetadata").getBoolValue();
    }

    if ((V == 4) || (V == 5))
    {
	QPDFObjectHandle CF = encryption_dict.getKey("/CF");
	std::set<std::string> keys = CF.getKeys();
	for (std::set<std::string>::iterator iter = keys.begin();
	     iter != keys.end(); ++iter)
	{
	    std::string const& filter = *iter;
	    QPDFObjectHandle cdict = CF.getKey(filter);
	    if (cdict.isDictionary())
	    {
		encryption_method_e method = e_none;
		if (cdict.getKey("/CFM").isName())
		{
		    std::string method_name = cdict.getKey("/CFM").getName();
		    if (method_name == "/V2")
		    {
			QTC::TC("qpdf", "QPDF_encryption CFM V2");
			method = e_rc4;
		    }
		    else if (method_name == "/AESV2")
		    {
			QTC::TC("qpdf", "QPDF_encryption CFM AESV2");
			method = e_aes;
		    }
		    else if (method_name == "/AESV3")
		    {
			QTC::TC("qpdf", "QPDF_encryption CFM AESV3");
			method = e_aesv3;
		    }
		    else
		    {
			// Don't complain now -- maybe we won't need
			// to reference this type.
			method = e_unknown;
		    }
		}
		this->m->crypt_filters[filter] = method;
	    }
	}

	QPDFObjectHandle StmF = encryption_dict.getKey("/StmF");
	QPDFObjectHandle StrF = encryption_dict.getKey("/StrF");
	QPDFObjectHandle EFF = encryption_dict.getKey("/EFF");
	this->m->cf_stream = interpretCF(StmF);
	this->m->cf_string = interpretCF(StrF);
	if (EFF.isName())
	{
	    this->m->cf_file = interpretCF(EFF);
	}
	else
	{
	    this->m->cf_file = this->m->cf_stream;
	}
    }

    EncryptionData data(V, R, Length / 8, P, O, U, OE, UE, Perms,
                        id1, this->m->encrypt_metadata);
    if (check_owner_password(
	    this->m->user_password, this->m->provided_password, data))
    {
	// password supplied was owner password; user_password has
	// been initialized for V < 5
    }
    else if (check_user_password(this->m->provided_password, data))
    {
	this->m->user_password = this->m->provided_password;
    }
    else
    {
	throw QPDFExc(qpdf_e_password, this->m->file->getName(),
		      "", 0, "invalid password");
    }

    if (V < 5)
    {
        // For V < 5, the user password is encrypted with the owner
        // password, and the user password is always used for
        // computing the encryption key.
        this->m->encryption_key = compute_encryption_key(
            this->m->user_password, data);
    }
    else
    {
        // For V >= 5, either password can be used independently to
        // compute the encryption key, and neither password can be
        // used to recover the other.
        bool perms_valid;
        this->m->encryption_key = recover_encryption_key_with_password(
            this->m->provided_password, data, perms_valid);
        if (! perms_valid)
        {
            warn(QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                         "encryption dictionary",
                         this->m->file->getLastOffset(),
                         "/Perms field in encryption dictionary"
                         " doesn't match expected value"));
        }
    }
}