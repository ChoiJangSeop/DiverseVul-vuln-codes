QPDFWriter::doWriteSetup()
{
    if (this->m->did_write_setup)
    {
        return;
    }
    this->m->did_write_setup = true;

    // Do preliminary setup

    if (this->m->linearized)
    {
	this->m->qdf_mode = false;
    }

    if (this->m->pclm)
    {
        this->m->stream_decode_level = qpdf_dl_none;
        this->m->compress_streams = false;
        this->m->encrypted = false;
    }

    if (this->m->qdf_mode)
    {
	if (! this->m->normalize_content_set)
	{
	    this->m->normalize_content = true;
	}
	if (! this->m->compress_streams_set)
	{
	    this->m->compress_streams = false;
	}
        if (! this->m->stream_decode_level_set)
        {
            this->m->stream_decode_level = qpdf_dl_generalized;
        }
    }

    if (this->m->encrypted)
    {
	// Encryption has been explicitly set
	this->m->preserve_encryption = false;
    }
    else if (this->m->normalize_content ||
	     this->m->stream_decode_level ||
             this->m->pclm ||
	     this->m->qdf_mode)
    {
	// Encryption makes looking at contents pretty useless.  If
	// the user explicitly encrypted though, we still obey that.
	this->m->preserve_encryption = false;
    }

    if (this->m->preserve_encryption)
    {
	copyEncryptionParameters(this->m->pdf);
    }

    if (! this->m->forced_pdf_version.empty())
    {
	int major = 0;
	int minor = 0;
	parseVersion(this->m->forced_pdf_version, major, minor);
	disableIncompatibleEncryption(major, minor,
                                      this->m->forced_extension_level);
	if (compareVersions(major, minor, 1, 5) < 0)
	{
	    QTC::TC("qpdf", "QPDFWriter forcing object stream disable");
	    this->m->object_stream_mode = qpdf_o_disable;
	}
    }

    if (this->m->qdf_mode || this->m->normalize_content ||
        this->m->stream_decode_level)
    {
	initializeSpecialStreams();
    }

    if (this->m->qdf_mode)
    {
	// Generate indirect stream lengths for qdf mode since fix-qdf
	// uses them for storing recomputed stream length data.
	// Certain streams such as object streams, xref streams, and
	// hint streams always get direct stream lengths.
	this->m->direct_stream_lengths = false;
    }

    switch (this->m->object_stream_mode)
    {
      case qpdf_o_disable:
	// no action required
	break;

      case qpdf_o_preserve:
	preserveObjectStreams();
	break;

      case qpdf_o_generate:
	generateObjectStreams();
	break;

	// no default so gcc will warn for missing case tag
    }

    if (this->m->linearized)
    {
	// Page dictionaries are not allowed to be compressed objects.
	std::vector<QPDFObjectHandle> pages = this->m->pdf.getAllPages();
	for (std::vector<QPDFObjectHandle>::iterator iter = pages.begin();
	     iter != pages.end(); ++iter)
	{
	    QPDFObjectHandle& page = *iter;
	    QPDFObjGen og = page.getObjGen();
	    if (this->m->object_to_object_stream.count(og))
	    {
		QTC::TC("qpdf", "QPDFWriter uncompressing page dictionary");
		this->m->object_to_object_stream.erase(og);
	    }
	}
    }

    if (this->m->linearized || this->m->encrypted)
    {
    	// The document catalog is not allowed to be compressed in
    	// linearized files either.  It also appears that Adobe Reader
    	// 8.0.0 has a bug that prevents it from being able to handle
    	// encrypted files with compressed document catalogs, so we
    	// disable them in that case as well.
	QPDFObjGen og = this->m->pdf.getRoot().getObjGen();
	if (this->m->object_to_object_stream.count(og))
	{
	    QTC::TC("qpdf", "QPDFWriter uncompressing root");
	    this->m->object_to_object_stream.erase(og);
	}
    }

    // Generate reverse mapping from object stream to objects
    for (std::map<QPDFObjGen, int>::iterator iter =
	     this->m->object_to_object_stream.begin();
	 iter != this->m->object_to_object_stream.end(); ++iter)
    {
	QPDFObjGen obj = (*iter).first;
	int stream = (*iter).second;
	this->m->object_stream_to_objects[stream].insert(obj);
	this->m->max_ostream_index =
	    std::max(this->m->max_ostream_index,
		     static_cast<int>(
                         this->m->object_stream_to_objects[stream].size()) - 1);
    }

    if (! this->m->object_stream_to_objects.empty())
    {
	setMinimumPDFVersion("1.5");
    }

    setMinimumPDFVersion(this->m->pdf.getPDFVersion(),
                         this->m->pdf.getExtensionLevel());
    this->m->final_pdf_version = this->m->min_pdf_version;
    this->m->final_extension_level = this->m->min_extension_level;
    if (! this->m->forced_pdf_version.empty())
    {
	QTC::TC("qpdf", "QPDFWriter using forced PDF version");
	this->m->final_pdf_version = this->m->forced_pdf_version;
        this->m->final_extension_level = this->m->forced_extension_level;
    }
}