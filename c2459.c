get16 (const byte *buffer)
{
  ulong a;
  a =  *buffer << 8;
  a |= buffer[1];
  return a;
}