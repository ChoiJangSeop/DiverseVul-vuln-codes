get32 (const byte *buffer)
{
  ulong a;
  a =  *buffer << 24;
  a |= buffer[1] << 16;
  a |= buffer[2] << 8;
  a |= buffer[3];
  return a;
}