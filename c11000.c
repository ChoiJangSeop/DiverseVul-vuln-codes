load_cache (GeglProperties *op_magick_load)
{
  if (!op_magick_load->user_data)
    {
      gchar    *filename;
      gchar    *cmd;
      GeglNode *graph, *sink, *loader;
      GeglBuffer *newbuf = NULL;

      /* ImageMagick backed fallback FIXME: make this robust.
       * maybe use pipes in a manner similar to the raw loader,
       * or at least use a properly unique filename  */

      filename = g_build_filename (g_get_tmp_dir (), "gegl-magick.png", NULL);
      cmd = g_strdup_printf ("convert \"%s\"'[0]' \"%s\"",
                             op_magick_load->path, filename);
      if (system (cmd) == -1)
        g_warning ("Error executing ImageMagick convert program");


      graph = gegl_node_new ();
      sink = gegl_node_new_child (graph,
                                 "operation", "gegl:buffer-sink",
                                 "buffer", &newbuf, NULL);
      loader = gegl_node_new_child (graph,
                                    "operation", "gegl:png-load",
                                    "path", filename, NULL);
      gegl_node_link_many (loader, sink, NULL);
      gegl_node_process (sink);
      op_magick_load->user_data = (gpointer) newbuf;
      g_object_unref (graph);
      g_free (cmd);
      g_free (filename);
    }
}