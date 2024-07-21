enable_xterm_mouse(SCREEN *sp, int enable)
{
#if USE_EMX_MOUSE
    sp->_emxmouse_activated = enable;
#else
    NCURSES_PUTP2("xterm-mouse", TPARM_1(sp->_mouse_xtermcap, enable));
#endif
    sp->_mouse_active = enable;
}