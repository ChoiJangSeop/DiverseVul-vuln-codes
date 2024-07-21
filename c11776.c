dbcs_ptr2len(
    char_u	*p)
{
    int		len;

    // Check if second byte is not missing.
    len = MB_BYTE2LEN(*p);
    if (len == 2 && p[1] == NUL)
	len = 1;
    return len;
}