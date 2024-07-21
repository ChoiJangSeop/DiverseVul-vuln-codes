didset_options2(void)
{
    // Initialize the highlight_attr[] table.
    (void)highlight_changed();

    // Parse default for 'wildmode'
    check_opt_wim();

    // Parse default for 'listchars'.
    (void)set_chars_option(curwin, &curwin->w_p_lcs);

    // Parse default for 'fillchars'.
    (void)set_chars_option(curwin, &p_fcs);

#ifdef FEAT_CLIPBOARD
    // Parse default for 'clipboard'
    (void)check_clipboard_option();
#endif
#ifdef FEAT_VARTABS
    vim_free(curbuf->b_p_vsts_array);
    tabstop_set(curbuf->b_p_vsts, &curbuf->b_p_vsts_array);
    vim_free(curbuf->b_p_vts_array);
    tabstop_set(curbuf->b_p_vts,  &curbuf->b_p_vts_array);
#endif
}