string PacketReader::getLabel(unsigned int recurs)
{
  string ret;
  ret.reserve(40);
  getLabelFromContent(d_content, d_pos, ret, recurs++);
  return ret;
}