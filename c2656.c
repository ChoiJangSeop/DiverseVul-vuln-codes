static gboolean printable_str(const char *str)
{
    do {
        if ((unsigned char)(*str) < ' ' || *str == 0x7f)
            return FALSE;
        str++;
    } while (*str);
    return TRUE;
}