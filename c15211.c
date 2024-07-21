QPDF::decryptStream(PointerHolder<EncryptionParameters> encp,
                    PointerHolder<InputSource> file,
                    QPDF& qpdf_for_warning, Pipeline*& pipeline,
                    int objid, int generation,
		    QPDFObjectHandle& stream_dict,
                    bool is_attachment_stream,
		    std::vector<PointerHolder<Pipeline> >& heap)
{
    std::string type;
    if (stream_dict.getKey("/Type").isName())
    {
	type = stream_dict.getKey("/Type").getName();
    }
    if (type == "/XRef")
    {
	QTC::TC("qpdf", "QPDF_encryption xref stream from encrypted file");
	return;
    }
    bool use_aes = false;
    if (encp->encryption_V >= 4)
    {
	encryption_method_e method = e_unknown;
	std::string method_source = "/StmF from /Encrypt dictionary";

	if (stream_dict.getKey("/Filter").isOrHasName("/Crypt"))
        {
            if (stream_dict.getKey("/DecodeParms").isDictionary())
            {
                QPDFObjectHandle decode_parms =
                    stream_dict.getKey("/DecodeParms");
                if (decode_parms.getKey("/Type").isName() &&
                    (decode_parms.getKey("/Type").getName() ==
                     "/CryptFilterDecodeParms"))
                {
                    QTC::TC("qpdf", "QPDF_encryption stream crypt filter");
                    method = interpretCF(encp, decode_parms.getKey("/Name"));
                    method_source = "stream's Crypt decode parameters";
                }
            }
            else if (stream_dict.getKey("/DecodeParms").isArray() &&
                     stream_dict.getKey("/Filter").isArray())
            {
                QPDFObjectHandle filter = stream_dict.getKey("/Filter");
                QPDFObjectHandle decode = stream_dict.getKey("/DecodeParms");
                if (filter.getArrayNItems() == decode.getArrayNItems())
                {
                    for (int i = 0; i < filter.getArrayNItems(); ++i)
                    {
                        if (filter.getArrayItem(i).isName() &&
                            (filter.getArrayItem(i).getName() == "/Crypt"))
                        {
                            QPDFObjectHandle crypt_params =
                                decode.getArrayItem(i);
                            if (crypt_params.isDictionary() &&
                                crypt_params.getKey("/Name").isName())
                            {
                                QTC::TC("qpdf", "QPDF_encrypt crypt array");
                                method = interpretCF(
                                    encp, crypt_params.getKey("/Name"));
                                method_source = "stream's Crypt "
                                    "decode parameters (array)";
                            }
                        }
                    }
                }
            }
	}

	if (method == e_unknown)
	{
	    if ((! encp->encrypt_metadata) && (type == "/Metadata"))
	    {
		QTC::TC("qpdf", "QPDF_encryption cleartext metadata");
		method = e_none;
	    }
	    else
	    {
                if (is_attachment_stream)
                {
                    QTC::TC("qpdf", "QPDF_encryption attachment stream");
                    method = encp->cf_file;
                }
                else
                {
                    method = encp->cf_stream;
                }
	    }
	}
	use_aes = false;
	switch (method)
	{
	  case e_none:
	    return;
	    break;

	  case e_aes:
	    use_aes = true;
	    break;

	  case e_aesv3:
	    use_aes = true;
	    break;

	  case e_rc4:
	    break;

	  default:
	    // filter local to this stream.
	    qpdf_for_warning.warn(
                QPDFExc(qpdf_e_damaged_pdf, file->getName(),
                        "", file->getLastOffset(),
                        "unknown encryption filter for streams"
                        " (check " + method_source + ");"
                        " streams may be decrypted improperly"));
	    // To avoid repeated warnings, reset cf_stream.  Assume
	    // we'd want to use AES if V == 4.
	    encp->cf_stream = e_aes;
            use_aes = true;
	    break;
	}
    }
    std::string key = getKeyForObject(encp, objid, generation, use_aes);
    if (use_aes)
    {
	QTC::TC("qpdf", "QPDF_encryption aes decode stream");
	pipeline = new Pl_AES_PDF("AES stream decryption", pipeline,
				  false, QUtil::unsigned_char_pointer(key),
                                  key.length());
    }
    else
    {
	QTC::TC("qpdf", "QPDF_encryption rc4 decode stream");
	pipeline = new Pl_RC4("RC4 stream decryption", pipeline,
			      QUtil::unsigned_char_pointer(key),
                              key.length());
    }
    heap.push_back(pipeline);
}