read_32 (IOBUF inp)
{
  unsigned long a;
  a = iobuf_get_noeof (inp) << 24;
  a |= iobuf_get_noeof (inp) << 16;
  a |= iobuf_get_noeof (inp) << 8;
  a |= iobuf_get_noeof (inp);
  return a;
}