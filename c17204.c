save_text(const char *fmt, const char *s, int len)
{
    size_t s_len = strlen(s);
    if (len > (int) s_len)
	s_len = (size_t) len;

    get_space(s_len + 1);

    _nc_SPRINTF(TPS(out_buff) + TPS(out_used),
		_nc_SLIMIT(TPS(out_size) - TPS(out_used))
		fmt, s);
    TPS(out_used) += strlen(TPS(out_buff) + TPS(out_used));
}