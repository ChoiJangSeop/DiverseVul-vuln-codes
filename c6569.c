x_catch_free_colors(Display * dpy, XErrorEvent * err)
{
    if (err->request_code == X_FreeColors)
        return 0;
    return x_error_handler.orighandler(dpy, err);
}