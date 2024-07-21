static void window_create(IMAGE *img)
{
    /* Create a gtk window */
    img->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(img->window), "gs");
    img->vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(img->window), img->vbox);
    gtk_widget_show(img->vbox);

    img->darea = gtk_drawing_area_new();
    gtk_widget_show(img->darea);
    img->scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(img->scroll);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(img->scroll),
        GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(img->scroll),
        img->darea);
    gtk_box_pack_start(GTK_BOX(img->vbox), img->scroll, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (img->darea), "expose-event",
                        GTK_SIGNAL_FUNC (window_draw), img);
    gtk_signal_connect(GTK_OBJECT (img->window), "destroy",
                        GTK_SIGNAL_FUNC (window_destroy), img);
    /* do not show img->window until we know the image size */
}