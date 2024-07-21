DnDCreateRootStagingDirectory(void)
{
   const char *root;

   /*
    * DnD_GetFileRoot() gives us a pointer to a static string, so there's no
    * need to free anything.
    */
   root = DnD_GetFileRoot();
   if (!root) {
      return NULL;
   }

   if (File_Exists(root)) {
      if (!DnDRootDirUsable(root) &&
          !DnDSetPermissionsOnRootDir(root)) {
         /*
          * The directory already exists and its permissions are wrong and
          * cannot be set, so there's not much we can do.
          */
         return NULL;
      }
   } else {
      if (!File_CreateDirectory(root) ||
          !DnDSetPermissionsOnRootDir(root)) {
         /* We couldn't create the directory or set the permissions. */
         return NULL;
      }
   }

   return root;
}