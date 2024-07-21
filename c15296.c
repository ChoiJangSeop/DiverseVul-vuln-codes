QPDF_Array::insertItem(int at, QPDFObjectHandle const& item)
{
    // As special case, also allow insert beyond the end
    if ((at < 0) || (at > static_cast<int>(this->items.size())))
    {
	throw std::logic_error(
	    "INTERNAL ERROR: bounds error accessing QPDF_Array element");
    }
    this->items.insert(this->items.begin() + at, item);
}