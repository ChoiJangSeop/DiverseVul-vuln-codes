CreateDir(const char* path)
{
   struct stat stats;
   char* token;
   char* copy;

   // make a copy we can modify
   copy = strdup(path);

   // walk through the path (it employs in string replacement)
   for (token = copy + 1; *token; ++token) {

      // find
      if (*token != '/') {
         continue;
      }

      /*
       * cut it off here for e.g. on first iteration /a/b/c/d.abc will have
       * token /a, on second /a/b etc
       */
      *token = 0;

      sLog(log_debug, "Creating directory %s", copy);

      // ignore if the directory exists
      if (!((stat(copy, &stats) == 0) && S_ISDIR(stats.st_mode))) {
         // make directory and check error (accessible only by owner)
         if (mkdir(copy, 0700) == -1) {
            sLog(log_error, "Unable to create directory %s (%s)", copy,
                 strerror(errno));
            free(copy);
            return FALSE;
         }
      }

      // restore the token
      *token = '/';
   }

   free(copy);
   return TRUE;
}