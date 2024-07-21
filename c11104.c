free_buf_options(
    buf_T	*buf,
    int		free_p_ff)
{
    if (free_p_ff)
    {
	clear_string_option(&buf->b_p_fenc);
	clear_string_option(&buf->b_p_ff);
	clear_string_option(&buf->b_p_bh);
	clear_string_option(&buf->b_p_bt);
    }
#ifdef FEAT_FIND_ID
    clear_string_option(&buf->b_p_def);
    clear_string_option(&buf->b_p_inc);
# ifdef FEAT_EVAL
    clear_string_option(&buf->b_p_inex);
# endif
#endif
#if defined(FEAT_CINDENT) && defined(FEAT_EVAL)
    clear_string_option(&buf->b_p_inde);
    clear_string_option(&buf->b_p_indk);
#endif
#if defined(FEAT_BEVAL) && defined(FEAT_EVAL)
    clear_string_option(&buf->b_p_bexpr);
#endif
#if defined(FEAT_CRYPT)
    clear_string_option(&buf->b_p_cm);
#endif
    clear_string_option(&buf->b_p_fp);
#if defined(FEAT_EVAL)
    clear_string_option(&buf->b_p_fex);
#endif
#ifdef FEAT_CRYPT
# ifdef FEAT_SODIUM
    if ((buf->b_p_key != NULL) && (*buf->b_p_key != NUL) &&
				(crypt_get_method_nr(buf) == CRYPT_M_SOD))
	crypt_sodium_munlock(buf->b_p_key, STRLEN(buf->b_p_key));
# endif
    clear_string_option(&buf->b_p_key);
#endif
    clear_string_option(&buf->b_p_kp);
    clear_string_option(&buf->b_p_mps);
    clear_string_option(&buf->b_p_fo);
    clear_string_option(&buf->b_p_flp);
    clear_string_option(&buf->b_p_isk);
#ifdef FEAT_VARTABS
    clear_string_option(&buf->b_p_vsts);
    vim_free(buf->b_p_vsts_nopaste);
    buf->b_p_vsts_nopaste = NULL;
    vim_free(buf->b_p_vsts_array);
    buf->b_p_vsts_array = NULL;
    clear_string_option(&buf->b_p_vts);
    VIM_CLEAR(buf->b_p_vts_array);
#endif
#ifdef FEAT_KEYMAP
    clear_string_option(&buf->b_p_keymap);
    keymap_clear(&buf->b_kmap_ga);
    ga_clear(&buf->b_kmap_ga);
#endif
    clear_string_option(&buf->b_p_com);
#ifdef FEAT_FOLDING
    clear_string_option(&buf->b_p_cms);
#endif
    clear_string_option(&buf->b_p_nf);
#ifdef FEAT_SYN_HL
    clear_string_option(&buf->b_p_syn);
    clear_string_option(&buf->b_s.b_syn_isk);
#endif
#ifdef FEAT_SPELL
    clear_string_option(&buf->b_s.b_p_spc);
    clear_string_option(&buf->b_s.b_p_spf);
    vim_regfree(buf->b_s.b_cap_prog);
    buf->b_s.b_cap_prog = NULL;
    clear_string_option(&buf->b_s.b_p_spl);
    clear_string_option(&buf->b_s.b_p_spo);
#endif
#ifdef FEAT_SEARCHPATH
    clear_string_option(&buf->b_p_sua);
#endif
    clear_string_option(&buf->b_p_ft);
#ifdef FEAT_CINDENT
    clear_string_option(&buf->b_p_cink);
    clear_string_option(&buf->b_p_cino);
#endif
#if defined(FEAT_CINDENT) || defined(FEAT_SMARTINDENT)
    clear_string_option(&buf->b_p_cinw);
#endif
    clear_string_option(&buf->b_p_cpt);
#ifdef FEAT_COMPL_FUNC
    clear_string_option(&buf->b_p_cfu);
    free_callback(&buf->b_cfu_cb);
    clear_string_option(&buf->b_p_ofu);
    free_callback(&buf->b_ofu_cb);
    clear_string_option(&buf->b_p_tsrfu);
    free_callback(&buf->b_tsrfu_cb);
#endif
#ifdef FEAT_QUICKFIX
    clear_string_option(&buf->b_p_gp);
    clear_string_option(&buf->b_p_mp);
    clear_string_option(&buf->b_p_efm);
#endif
    clear_string_option(&buf->b_p_ep);
    clear_string_option(&buf->b_p_path);
    clear_string_option(&buf->b_p_tags);
    clear_string_option(&buf->b_p_tc);
#ifdef FEAT_EVAL
    clear_string_option(&buf->b_p_tfu);
    free_callback(&buf->b_tfu_cb);
#endif
    clear_string_option(&buf->b_p_dict);
    clear_string_option(&buf->b_p_tsr);
#ifdef FEAT_TEXTOBJ
    clear_string_option(&buf->b_p_qe);
#endif
    buf->b_p_ar = -1;
    buf->b_p_ul = NO_LOCAL_UNDOLEVEL;
#ifdef FEAT_LISP
    clear_string_option(&buf->b_p_lw);
#endif
    clear_string_option(&buf->b_p_bkc);
    clear_string_option(&buf->b_p_menc);
}