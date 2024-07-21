QPDFWriter::writeObjectStreamOffsets(std::vector<qpdf_offset_t>& offsets,
				     int first_obj)
{
    for (unsigned int i = 0; i < offsets.size(); ++i)
    {
	if (i != 0)
	{
	    writeStringQDF("\n");
	    writeStringNoQDF(" ");
	}
	writeString(QUtil::int_to_string(i + first_obj));
	writeString(" ");
	writeString(QUtil::int_to_string(offsets.at(i)));
    }
    writeString("\n");
}