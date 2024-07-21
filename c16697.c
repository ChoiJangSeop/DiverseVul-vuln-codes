rsvg_state_finalize (RsvgState * state)
{
    g_free (state->font_family);
    g_free (state->lang);
    rsvg_paint_server_unref (state->fill);
    rsvg_paint_server_unref (state->stroke);

    if (state->dash.n_dash != 0)
        g_free (state->dash.dash);

    if (state->styles) {
        g_hash_table_unref (state->styles);
        state->styles = NULL;
    }
}