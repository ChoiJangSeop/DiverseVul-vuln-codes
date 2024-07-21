on_screen_monitors_changed (GdkScreen *screen,
                            GSManager *manager)
{
        gs_debug ("Monitors changed for screen %d: num=%d",
                  gdk_screen_get_number (screen),
                  gdk_screen_get_n_monitors (screen));
}