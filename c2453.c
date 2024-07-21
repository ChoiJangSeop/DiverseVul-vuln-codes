read_16 (IOBUF inp)
{
  unsigned short a;
  a = iobuf_get_noeof (inp) << 8;
  a |= iobuf_get_noeof (inp);
  return a;
}