void gtkui_icmp_redir(void)
{
   GtkWidget *dialog, *table, *hbox, *image, *label, *entry1, *entry2, *frame;
   gint response = 0;

   DEBUG_MSG("gtk_icmp_redir");

   dialog = gtk_dialog_new_with_buttons("MITM Attack: ICMP Redirect", GTK_WINDOW (window),
                                        GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
   gtk_container_set_border_width(GTK_CONTAINER (dialog), 5);
   gtk_dialog_set_has_separator(GTK_DIALOG (dialog), FALSE);

   hbox = gtk_hbox_new (FALSE, 5);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show(hbox);

   image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
   gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.1);
   gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 5);
   gtk_widget_show(image);

   frame = gtk_frame_new("Gateway Information");
   gtk_container_set_border_width(GTK_CONTAINER (frame), 5);
   gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
   gtk_widget_show(frame);

   table = gtk_table_new(2, 2, FALSE);
   gtk_table_set_row_spacings(GTK_TABLE (table), 5);
   gtk_table_set_col_spacings(GTK_TABLE (table), 5);
   gtk_container_set_border_width(GTK_CONTAINER (table), 8);
   gtk_container_add(GTK_CONTAINER (frame), table);
   gtk_widget_show(table);

   label = gtk_label_new("MAC Address");
   gtk_misc_set_alignment(GTK_MISC (label), 0, 0.5);
   gtk_table_attach(GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_show(label);

   entry1 = gtk_entry_new();
   gtk_entry_set_max_length(GTK_ENTRY (entry1), ETH_ASCII_ADDR_LEN);
   gtk_table_attach_defaults(GTK_TABLE (table), entry1, 1, 2, 0, 1); 
   gtk_widget_show(entry1);

   label = gtk_label_new("IP Address");
   gtk_misc_set_alignment(GTK_MISC (label), 0, 0.5);
   gtk_table_attach(GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_show(label);

   entry2 = gtk_entry_new();
   gtk_entry_set_max_length(GTK_ENTRY (entry2), IP6_ASCII_ADDR_LEN);
   gtk_table_attach_defaults(GTK_TABLE (table), entry2, 1, 2, 1, 2);
   gtk_widget_show(entry2);

   response = gtk_dialog_run(GTK_DIALOG(dialog));
   if(response == GTK_RESPONSE_OK) {
      gtk_widget_hide(dialog);

      snprintf(params, 6, "icmp:");

      strncat(params, gtk_entry_get_text(GTK_ENTRY(entry1)), PARAMS_LEN);
      strncat(params, "/", PARAMS_LEN);
      strncat(params, gtk_entry_get_text(GTK_ENTRY(entry2)), PARAMS_LEN);

      gtkui_start_mitm();
   }

   gtk_widget_destroy(dialog);

   /* a simpler method:
      gtkui_input_call("Parameters :", params + strlen("icmp:"), PARAMS_LEN - strlen("icmp:"), gtkui_start_mitm);
    */
}