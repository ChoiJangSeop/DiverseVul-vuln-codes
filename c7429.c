CreateApparentRootDirectory(void)
{
   const char *root;

   /*
    * DnD_GetFileRoot() gives us a pointer to a static string, so there's no
    * need to free anything.
    *
    * XXX On XDG platforms this path ("/tmp/VMwareDnD") is created by an
    *     init script, so we could remove some of the code below and just bail
    *     if the user deletes it.
    */

   root = DnD_GetFileRoot();
   if (!root) {
      return NULL;
   }

   if (File_Exists(root)) {
      if (   !DnDRootDirUsable(root)
          && !DnDSetPermissionsOnRootDir(root)) {
         /*
          * The directory already exists and its permissions are wrong and
          * cannot be set, so there's not much we can do.
          */
         return NULL;
      }
   } else {
      if (   !File_CreateDirectory(root)
          || !DnDSetPermissionsOnRootDir(root)) {
         /* We couldn't create the directory or set the permissions. */
         return NULL;
      }
   }

   return root;
}