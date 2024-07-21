write_vector_vector(BitWriter& w,
		    int nitems1, std::vector<T>& vec1, int T::*nitems2,
		    int bits, std::vector<int> T::*vec2)
{
    // nitems1 times, write nitems2 (from the ith element of vec1) items
    // from the vec2 vector field of the ith item of vec1.
    for (int i1 = 0; i1 < nitems1; ++i1)
    {
	for (int i2 = 0; i2 < vec1.at(i1).*nitems2; ++i2)
	{
	    w.writeBits((vec1.at(i1).*vec2).at(i2), bits);
	}
    }
    w.flush();
}