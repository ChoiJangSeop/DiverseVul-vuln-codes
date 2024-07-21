set_vterm_palette(VTerm *vterm, long_u *rgb)
{
    int		index = 0;
    VTermState	*state = vterm_obtain_state(vterm);
    for (; index < 16; index++)
    {
	VTermColor	color;
	color.red = (unsigned)(rgb[index] >> 16);
	color.green = (unsigned)(rgb[index] >> 8) & 255;
	color.blue = (unsigned)rgb[index] & 255;
	vterm_state_set_palette_color(state, index, &color);
    }
}