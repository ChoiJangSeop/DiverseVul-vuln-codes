write_vector_int(BitWriter& w, int nitems, std::vector<T>& vec,
		 int bits, int_type T::*field)
{
    // nitems times, write bits bits from the given field of the ith
    // vector to the given bit writer.

    for (int i = 0; i < nitems; ++i)
    {
	w.writeBits(vec.at(i).*field, bits);
    }
    // The PDF spec says that each hint table starts at a byte
    // boundary.  Each "row" actually must start on a byte boundary.
    w.flush();
}