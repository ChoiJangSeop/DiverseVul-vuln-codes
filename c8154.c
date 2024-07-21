postprocess_terminfo(TERMTYPE2 *tp)
{
    /*
     * TERMINFO-TO-TERMINFO MAPPINGS FOR SOURCE TRANSLATION
     * ----------------------------------------------------------------------
     */

    /*
     * Translate AIX forms characters.
     */
    if (PRESENT(box_chars_1)) {
	char buf2[MAX_TERMCAP_LENGTH];
	string_desc result;

	_nc_str_init(&result, buf2, sizeof(buf2));
	_nc_safe_strcat(&result, acs_chars);

	append_acs0(&result, 'l', box_chars_1[0]);	/* ACS_ULCORNER */
	append_acs0(&result, 'q', box_chars_1[1]);	/* ACS_HLINE */
	append_acs0(&result, 'k', box_chars_1[2]);	/* ACS_URCORNER */
	append_acs0(&result, 'x', box_chars_1[3]);	/* ACS_VLINE */
	append_acs0(&result, 'j', box_chars_1[4]);	/* ACS_LRCORNER */
	append_acs0(&result, 'm', box_chars_1[5]);	/* ACS_LLCORNER */
	append_acs0(&result, 'w', box_chars_1[6]);	/* ACS_TTEE */
	append_acs0(&result, 'u', box_chars_1[7]);	/* ACS_RTEE */
	append_acs0(&result, 'v', box_chars_1[8]);	/* ACS_BTEE */
	append_acs0(&result, 't', box_chars_1[9]);	/* ACS_LTEE */
	append_acs0(&result, 'n', box_chars_1[10]);	/* ACS_PLUS */

	if (buf2[0]) {
	    acs_chars = _nc_save_str(buf2);
	    _nc_warning("acsc string synthesized from AIX capabilities");
	    box_chars_1 = ABSENT_STRING;
	}
    }
    /*
     * ----------------------------------------------------------------------
     */
}