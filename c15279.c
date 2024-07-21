QPDFXRefEntry::getObjStreamNumber() const
{
    if (this->type != 2)
    {
	throw std::logic_error(
	    "getObjStreamNumber called for xref entry of type != 2");
    }
    return this->field1;
}