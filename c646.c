gdk_pixbuf__tiff_fill_vtable (GdkPixbufModule *module)
{
  module->load = gdk_pixbuf__tiff_image_load;
  module->begin_load = gdk_pixbuf__tiff_image_begin_load;
  module->stop_load = gdk_pixbuf__tiff_image_stop_load;
  module->load_increment = gdk_pixbuf__tiff_image_load_increment;
}