QPDFWriter::generateObjectStreams()
{
    // Basic strategy: make a list of objects that can go into an
    // object stream.  Then figure out how many object streams are
    // needed so that we can distribute objects approximately evenly
    // without having any object stream exceed 100 members.  We don't
    // have to worry about linearized files here -- if the file is
    // linearized, we take care of excluding things that aren't
    // allowed here later.

    // This code doesn't do anything with /Extends.

    std::vector<QPDFObjGen> const& eligible =
        QPDF::Writer::getCompressibleObjGens(this->m->pdf);
    unsigned int n_object_streams = (eligible.size() + 99) / 100;
    if (n_object_streams == 0)
    {
        return;
    }
    unsigned int n_per = eligible.size() / n_object_streams;
    if (n_per * n_object_streams < eligible.size())
    {
	++n_per;
    }
    unsigned int n = 0;
    int cur_ostream = 0;
    for (std::vector<QPDFObjGen>::const_iterator iter = eligible.begin();
	 iter != eligible.end(); ++iter)
    {
	if ((n % n_per) == 0)
	{
	    if (n > 0)
	    {
		QTC::TC("qpdf", "QPDFWriter generate >1 ostream");
	    }
	    n = 0;
	}
	if (n == 0)
	{
	    // Construct a new null object as the "original" object
	    // stream.  The rest of the code knows that this means
	    // we're creating the object stream from scratch.
	    cur_ostream = this->m->pdf.makeIndirectObject(
		QPDFObjectHandle::newNull()).getObjectID();
	}
	this->m->object_to_object_stream[*iter] = cur_ostream;
	++n;
    }
}