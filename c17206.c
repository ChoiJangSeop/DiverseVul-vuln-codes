_nc_free_tparm(void)
{
    if (TPS(out_buff) != 0) {
	FreeAndNull(TPS(out_buff));
	TPS(out_size) = 0;
	TPS(out_used) = 0;
	FreeAndNull(TPS(fmt_buff));
	TPS(fmt_size) = 0;
    }
}