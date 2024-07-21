QPDF::outputLengthNextN(
    int in_object, int n,
    std::map<int, qpdf_offset_t> const& lengths,
    std::map<int, int> const& obj_renumber)
{
    // Figure out the length of a series of n consecutive objects in
    // the output file starting with whatever object in_object from
    // the input file mapped to.

    if (obj_renumber.count(in_object) == 0)
    {
        stopOnError("found object that is not renumbered while"
                    " writing linearization data");
    }
    int first = (*(obj_renumber.find(in_object))).second;
    int length = 0;
    for (int i = 0; i < n; ++i)
    {
	if (lengths.count(first + i) == 0)
        {
            stopOnError("found item with unknown length"
                        " while writing linearization data");
        }
	length += (*(lengths.find(first + i))).second;
    }
    return length;
}