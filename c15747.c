void gtkui_conf_read(void) {
   FILE *fd;
   const char *path;
   char line[100], name[30];
   short value;

#ifdef OS_WINDOWS
   path = ec_win_get_user_dir();
#else
   /* TODO: get the dopped privs home dir instead of "/root" */
   /* path = g_get_home_dir(); */
   path = g_get_tmp_dir();
#endif

   filename = g_build_filename(path, ".ettercap_gtk", NULL);

   DEBUG_MSG("gtkui_conf_read: %s", filename);

   fd = fopen(filename, "r");
   if(!fd) 
      return;

   while(fgets(line, 100, fd)) {
      sscanf(line, "%s = %hd", name, &value);

      gtkui_conf_set(name, value);
   }
   fclose(fd);
}