append_acs0(string_desc * dst, int code, int src)
{
    if (src != 0) {
	char temp[3];
	temp[0] = (char) code;
	temp[1] = (char) src;
	temp[2] = 0;
	_nc_safe_strcat(dst, temp);
    }
}