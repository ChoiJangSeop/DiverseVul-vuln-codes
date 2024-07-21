QPDFWriter::bytesNeeded(unsigned long long n)
{
    int bytes = 0;
    while (n)
    {
	++bytes;
	n >>= 8;
    }
    return bytes;
}