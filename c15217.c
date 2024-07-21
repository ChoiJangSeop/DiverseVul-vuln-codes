QPDF::readLinearizationData()
{
    // This function throws an exception (which is trapped by
    // checkLinearization()) for any errors that prevent loading.

    // Hint table parsing code needs at least 32 bits in a long.
    assert(sizeof(long) >= 4);

    if (! isLinearized())
    {
	throw std::logic_error("called readLinearizationData for file"
			       " that is not linearized");
    }

    // /L is read and stored in linp by isLinearized()
    QPDFObjectHandle H = this->m->lindict.getKey("/H");
    QPDFObjectHandle O = this->m->lindict.getKey("/O");
    QPDFObjectHandle E = this->m->lindict.getKey("/E");
    QPDFObjectHandle N = this->m->lindict.getKey("/N");
    QPDFObjectHandle T = this->m->lindict.getKey("/T");
    QPDFObjectHandle P = this->m->lindict.getKey("/P");

    if (! (H.isArray() &&
	   O.isInteger() &&
	   E.isInteger() &&
	   N.isInteger() &&
	   T.isInteger() &&
	   (P.isInteger() || P.isNull())))
    {
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      "linearization dictionary",
		      this->m->file->getLastOffset(),
		      "some keys in linearization dictionary are of "
		      "the wrong type");
    }

    // Hint table array: offset length [ offset length ]
    unsigned int n_H_items = H.getArrayNItems();
    if (! ((n_H_items == 2) || (n_H_items == 4)))
    {
	throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
		      "linearization dictionary",
		      this->m->file->getLastOffset(),
		      "H has the wrong number of items");
    }

    std::vector<int> H_items;
    for (unsigned int i = 0; i < n_H_items; ++i)
    {
	QPDFObjectHandle oh(H.getArrayItem(i));
	if (oh.isInteger())
	{
	    H_items.push_back(oh.getIntValue());
	}
	else
	{
	    throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
			  "linearization dictionary",
			  this->m->file->getLastOffset(),
			  "some H items are of the wrong type");
	}
    }

    // H: hint table offset/length for primary and overflow hint tables
    int H0_offset = H_items.at(0);
    int H0_length = H_items.at(1);
    int H1_offset = 0;
    int H1_length = 0;
    if (H_items.size() == 4)
    {
	// Acrobat doesn't read or write these (as PDF 1.4), so we
	// don't have a way to generate a test case.
	// QTC::TC("qpdf", "QPDF overflow hint table");
	H1_offset = H_items.at(2);
	H1_length = H_items.at(3);
    }

    // P: first page number
    int first_page = 0;
    if (P.isInteger())
    {
	QTC::TC("qpdf", "QPDF P present in lindict");
	first_page = P.getIntValue();
    }
    else
    {
	QTC::TC("qpdf", "QPDF P absent in lindict");
    }

    // Store linearization parameter data

    // Various places in the code use linp.npages, which is
    // initialized from N, to pre-allocate memory, so make sure it's
    // accurate and bail right now if it's not.
    if (N.getIntValue() != static_cast<long long>(getAllPages().size()))
    {
        throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                      "linearization hint table",
                      this->m->file->getLastOffset(),
                      "/N does not match number of pages");
    }

    // file_size initialized by isLinearized()
    this->m->linp.first_page_object = O.getIntValue();
    this->m->linp.first_page_end = E.getIntValue();
    this->m->linp.npages = N.getIntValue();
    this->m->linp.xref_zero_offset = T.getIntValue();
    this->m->linp.first_page = first_page;
    this->m->linp.H_offset = H0_offset;
    this->m->linp.H_length = H0_length;

    // Read hint streams

    Pl_Buffer pb("hint buffer");
    QPDFObjectHandle H0 = readHintStream(pb, H0_offset, H0_length);
    if (H1_offset)
    {
	(void) readHintStream(pb, H1_offset, H1_length);
    }

    // PDF 1.4 hint tables that we ignore:

    //  /T    thumbnail
    //  /A    thread information
    //  /E    named destination
    //  /V    interactive form
    //  /I    information dictionary
    //  /C    logical structure
    //  /L    page label

    // Individual hint table offsets
    QPDFObjectHandle HS = H0.getKey("/S"); // shared object
    QPDFObjectHandle HO = H0.getKey("/O"); // outline

    PointerHolder<Buffer> hbp = pb.getBuffer();
    Buffer* hb = hbp.getPointer();
    unsigned char const* h_buf = hb->getBuffer();
    int h_size = hb->getSize();

    readHPageOffset(BitStream(h_buf, h_size));

    int HSi = HS.getIntValue();
    if ((HSi < 0) || (HSi >= h_size))
    {
        throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                      "linearization hint table",
                      this->m->file->getLastOffset(),
                      "/S (shared object) offset is out of bounds");
    }
    readHSharedObject(BitStream(h_buf + HSi, h_size - HSi));

    if (HO.isInteger())
    {
	int HOi = HO.getIntValue();
        if ((HOi < 0) || (HOi >= h_size))
        {
            throw QPDFExc(qpdf_e_damaged_pdf, this->m->file->getName(),
                          "linearization hint table",
                          this->m->file->getLastOffset(),
                          "/O (outline) offset is out of bounds");
        }
	readHGeneric(BitStream(h_buf + HOi, h_size - HOi),
		     this->m->outline_hints);
    }
}