ex_put(exarg_T *eap)
{
    // ":0put" works like ":1put!".
    if (eap->line2 == 0)
    {
	eap->line2 = 1;
	eap->forceit = TRUE;
    }
    curwin->w_cursor.lnum = eap->line2;
    do_put(eap->regname, NULL, eap->forceit ? BACKWARD : FORWARD, 1L,
						       PUT_LINE|PUT_CURSLINE);
}