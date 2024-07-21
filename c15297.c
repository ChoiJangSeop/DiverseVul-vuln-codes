BitStream::BitStream(unsigned char const* p, int nbytes) :
    start(p),
    nbytes(nbytes)
{
    reset();
}