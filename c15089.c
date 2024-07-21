static bool is_iso_latin1_printable(unsigned char ch)
{
    return (((ch >= 32) && (ch <= 126)) || (ch >= 160));
}