QPDF_Stream::pipeStreamData(Pipeline* pipeline,
                            int encode_flags,
                            qpdf_stream_decode_level_e decode_level,
                            bool suppress_warnings, bool will_retry)
{
    std::vector<std::string> filters;
    int predictor = 1;
    int columns = 0;
    int colors = 1;
    int bits_per_component = 8;
    bool early_code_change = true;
    bool specialized_compression = false;
    bool lossy_compression = false;
    bool filter = (! ((encode_flags == 0) && (decode_level == qpdf_dl_none)));
    if (filter)
    {
	filter = filterable(filters, specialized_compression, lossy_compression,
                            predictor, columns,
                            colors, bits_per_component,
                            early_code_change);
        if ((decode_level < qpdf_dl_all) && lossy_compression)
        {
            filter = false;
        }
        if ((decode_level < qpdf_dl_specialized) && specialized_compression)
        {
            filter = false;
        }
        QTC::TC("qpdf", "QPDF_Stream special filters",
                (! filter) ? 0 :
                lossy_compression ? 1 :
                specialized_compression ? 2 :
                3);
    }

    if (pipeline == 0)
    {
	QTC::TC("qpdf", "QPDF_Stream pipeStreamData with null pipeline");
	return filter;
    }

    // Construct the pipeline in reverse order.  Force pipelines we
    // create to be deleted when this function finishes.
    std::vector<PointerHolder<Pipeline> > to_delete;

    PointerHolder<ContentNormalizer> normalizer;
    if (filter)
    {
	if (encode_flags & qpdf_ef_compress)
	{
	    pipeline = new Pl_Flate("compress stream", pipeline,
				    Pl_Flate::a_deflate);
	    to_delete.push_back(pipeline);
	}

	if (encode_flags & qpdf_ef_normalize)
	{
            normalizer = new ContentNormalizer();
	    pipeline = new Pl_QPDFTokenizer(
                "normalizer", normalizer.getPointer(), pipeline);
	    to_delete.push_back(pipeline);
	}

        for (std::vector<PointerHolder<
                 QPDFObjectHandle::TokenFilter> >::reverse_iterator iter =
                 this->token_filters.rbegin();
             iter != this->token_filters.rend(); ++iter)
        {
            pipeline = new Pl_QPDFTokenizer(
                "token filter", (*iter).getPointer(), pipeline);
            to_delete.push_back(pipeline);
        }

	for (std::vector<std::string>::reverse_iterator iter = filters.rbegin();
	     iter != filters.rend(); ++iter)
	{
	    std::string const& filter = *iter;

            if ((filter == "/FlateDecode") || (filter == "/LZWDecode"))
            {
                if ((predictor >= 10) && (predictor <= 15))
                {
                    QTC::TC("qpdf", "QPDF_Stream PNG filter");
                    pipeline = new Pl_PNGFilter(
                        "png decode", pipeline, Pl_PNGFilter::a_decode,
                        columns, colors, bits_per_component);
                    to_delete.push_back(pipeline);
                }
                else if (predictor == 2)
                {
                    QTC::TC("qpdf", "QPDF_Stream TIFF predictor");
                    pipeline = new Pl_TIFFPredictor(
                        "tiff decode", pipeline, Pl_TIFFPredictor::a_decode,
                        columns, colors, bits_per_component);
                    to_delete.push_back(pipeline);
                }
            }

	    if (filter == "/Crypt")
	    {
		// Ignore -- handled by pipeStreamData
	    }
	    else if (filter == "/FlateDecode")
	    {
		pipeline = new Pl_Flate("stream inflate",
					pipeline, Pl_Flate::a_inflate);
		to_delete.push_back(pipeline);
	    }
	    else if (filter == "/ASCII85Decode")
	    {
		pipeline = new Pl_ASCII85Decoder("ascii85 decode", pipeline);
		to_delete.push_back(pipeline);
	    }
	    else if (filter == "/ASCIIHexDecode")
	    {
		pipeline = new Pl_ASCIIHexDecoder("asciiHex decode", pipeline);
		to_delete.push_back(pipeline);
	    }
	    else if (filter == "/LZWDecode")
	    {
		pipeline = new Pl_LZWDecoder("lzw decode", pipeline,
					     early_code_change);
		to_delete.push_back(pipeline);
	    }
	    else if (filter == "/RunLengthDecode")
	    {
		pipeline = new Pl_RunLength("runlength decode", pipeline,
                                            Pl_RunLength::a_decode);
		to_delete.push_back(pipeline);
	    }
	    else if (filter == "/DCTDecode")
	    {
		pipeline = new Pl_DCT("DCT decode", pipeline);
		to_delete.push_back(pipeline);
	    }
	    else
	    {
		throw std::logic_error(
		    "INTERNAL ERROR: QPDFStream: unknown filter "
		    "encountered after check");
	    }
	}
    }

    if (this->stream_data.getPointer())
    {
	QTC::TC("qpdf", "QPDF_Stream pipe replaced stream data");
	pipeline->write(this->stream_data->getBuffer(),
			this->stream_data->getSize());
	pipeline->finish();
    }
    else if (this->stream_provider.getPointer())
    {
	Pl_Count count("stream provider count", pipeline);
	this->stream_provider->provideStreamData(
	    this->objid, this->generation, &count);
	qpdf_offset_t actual_length = count.getCount();
	qpdf_offset_t desired_length = 0;
        if (this->stream_dict.hasKey("/Length"))
        {
	    desired_length = this->stream_dict.getKey("/Length").getIntValue();
            if (actual_length == desired_length)
            {
                QTC::TC("qpdf", "QPDF_Stream pipe use stream provider");
            }
            else
            {
                QTC::TC("qpdf", "QPDF_Stream provider length mismatch");
                // This would be caused by programmer error on the
                // part of a library user, not by invalid input data.
                throw std::runtime_error(
                    "stream data provider for " +
                    QUtil::int_to_string(this->objid) + " " +
                    QUtil::int_to_string(this->generation) +
                    " provided " +
                    QUtil::int_to_string(actual_length) +
                    " bytes instead of expected " +
                    QUtil::int_to_string(desired_length) + " bytes");
            }
        }
        else
        {
            QTC::TC("qpdf", "QPDF_Stream provider length not provided");
            this->stream_dict.replaceKey(
                "/Length", QPDFObjectHandle::newInteger(actual_length));
        }
    }
    else if (this->offset == 0)
    {
	QTC::TC("qpdf", "QPDF_Stream pipe no stream data");
	throw std::logic_error(
	    "pipeStreamData called for stream with no data");
    }
    else
    {
	QTC::TC("qpdf", "QPDF_Stream pipe original stream data");
	if (! QPDF::Pipe::pipeStreamData(this->qpdf, this->objid, this->generation,
                                         this->offset, this->length,
                                         this->stream_dict, pipeline,
                                         suppress_warnings,
                                         will_retry))
        {
            filter = false;
        }
    }

    if (filter &&
        (! suppress_warnings) &&
        normalizer.getPointer() &&
        normalizer->anyBadTokens())
    {
        warn(QPDFExc(qpdf_e_damaged_pdf, qpdf->getFilename(),
                     "", this->offset,
                     "content normalization encountered bad tokens"));
        if (normalizer->lastTokenWasBad())
        {
            QTC::TC("qpdf", "QPDF_Stream bad token at end during normalize");
            warn(QPDFExc(qpdf_e_damaged_pdf, qpdf->getFilename(),
                         "", this->offset,
                         "normalized content ended with a bad token;"
                         " you may be able to resolve this by"
                         " coalescing content streams in combination"
                         " with normalizing content. From the command"
                         " line, specify --coalesce-contents"));
        }
        warn(QPDFExc(qpdf_e_damaged_pdf, qpdf->getFilename(),
                     "", this->offset,
                     "Resulting stream data may be corrupted but is"
                     " may still useful for manual inspection."
                     " For more information on this warning, search"
                     " for content normalization in the manual."));
    }

    return filter;
}