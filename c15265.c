static void doubleBoxSize(QPDFObjectHandle& page, char const* box_name)
{
    // If there is a box of this name, replace it with a new box whose
    // elements are double the values of the original box.
    QPDFObjectHandle box = page.getKey(box_name);
    if (box.isNull())
    {
	return;
    }
    if (! (box.isArray() && (box.getArrayNItems() == 4)))
    {
	throw std::runtime_error(std::string("box ") + box_name +
				 " is not an array of four elements");
    }
    std::vector<QPDFObjectHandle> doubled;
    for (unsigned int i = 0; i < 4; ++i)
    {
	doubled.push_back(
	    QPDFObjectHandle::newReal(
                box.getArrayItem(i).getNumericValue() * 2.0, 2));
    }
    page.replaceKey(box_name, QPDFObjectHandle::newArray(doubled));
}