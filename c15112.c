QPDF_Array::getItem(int n) const
{
    if ((n < 0) || (n >= static_cast<int>(this->items.size())))
    {
	throw std::logic_error(
	    "INTERNAL ERROR: bounds error accessing QPDF_Array element");
    }
    return this->items.at(n);
}