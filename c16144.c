window_add_button(IMAGE *img, const char *label, GtkSignalFunc fn)
{
    GtkWidget *w;
    w = gtk_check_button_new_with_label(label);
    gtk_box_pack_start(GTK_BOX(img->cmyk_bar), w, FALSE, FALSE, 5);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
    gtk_signal_connect(GTK_OBJECT(w), "clicked", fn, img);
    gtk_widget_show(w);
    return w;
}