int ber_get_content_length(int length)
{
	if (length > 0x81)
		return length - 4;
	else
		return length - 2;
}