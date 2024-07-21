load_vector_int(BitStream& bit_stream, int nitems, std::vector<T>& vec,
		int bits_wanted, int_type T::*field)
{
    bool append = vec.empty();
    // nitems times, read bits_wanted from the given bit stream,
    // storing results in the ith vector entry.

    for (int i = 0; i < nitems; ++i)
    {
        if (append)
        {
            vec.push_back(T());
        }
	vec.at(i).*field = bit_stream.getBits(bits_wanted);
    }
    if (static_cast<int>(vec.size()) != nitems)
    {
        throw std::logic_error("vector has wrong size in load_vector_int");
    }
    // The PDF spec says that each hint table starts at a byte
    // boundary.  Each "row" actually must start on a byte boundary.
    bit_stream.skipToNextByte();
}