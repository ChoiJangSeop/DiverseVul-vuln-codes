QPDF::checkHOutlines(std::list<std::string>& warnings)
{
    // Empirically, Acrobat generates the correct value for the object
    // number but incorrectly stores the next object number's offset
    // as the offset, at least when outlines appear in part 6.  It
    // also generates an incorrect value for length (specifically, the
    // length that would cover the correct number of objects from the
    // wrong starting place).  pdlin appears to generate correct
    // values in those cases.

    if (this->m->c_outline_data.nobjects == this->m->outline_hints.nobjects)
    {
	if (this->m->c_outline_data.nobjects == 0)
	{
	    return;
	}

	if (this->m->c_outline_data.first_object ==
	    this->m->outline_hints.first_object)
	{
	    // Check length and offset.  Acrobat gets these wrong.
	    QPDFObjectHandle outlines = getRoot().getKey("/Outlines");
            if (! outlines.isIndirect())
            {
                // This case is not exercised in test suite since not
                // permitted by the spec, but if this does occur, the
                // code below would fail.
		warnings.push_back(
		    "/Outlines key of root dictionary is not indirect");
                return;
            }
	    QPDFObjGen og(outlines.getObjGen());
	    if (this->m->xref_table.count(og) == 0)
            {
                stopOnError("unknown object in outlines hint table");
            }
	    int offset = getLinearizationOffset(og);
	    ObjUser ou(ObjUser::ou_root_key, "/Outlines");
	    int length = maxEnd(ou) - offset;
	    int table_offset =
		adjusted_offset(this->m->outline_hints.first_object_offset);
	    if (offset != table_offset)
	    {
		warnings.push_back(
		    "incorrect offset in outlines table: hint table = " +
		    QUtil::int_to_string(table_offset) +
		    "; computed = " + QUtil::int_to_string(offset));
	    }
	    int table_length = this->m->outline_hints.group_length;
	    if (length != table_length)
	    {
		warnings.push_back(
		    "incorrect length in outlines table: hint table = " +
		    QUtil::int_to_string(table_length) +
		    "; computed = " + QUtil::int_to_string(length));
	    }
	}
	else
	{
	    warnings.push_back("incorrect first object number in outline "
			       "hints table.");
	}
    }
    else
    {
	warnings.push_back("incorrect object count in outline hint table");
    }
}