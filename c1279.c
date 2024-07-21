parse_suse_release (guestfs_h *g, struct inspect_fs *fs, const char *filename)
{
  int64_t size;
  char *major, *minor;
  CLEANUP_FREE_STRING_LIST char **lines = NULL;
  int r = -1;

  /* Don't trust guestfs_head_n not to break with very large files.
   * Check the file size is something reasonable first.
   */
  size = guestfs_filesize (g, filename);
  if (size == -1)
    /* guestfs_filesize failed and has already set error in handle */
    return -1;
  if (size > MAX_SMALL_FILE_SIZE) {
    error (g, _("size of %s is unreasonably large (%" PRIi64 " bytes)"),
           filename, size);
    return -1;
  }

  lines = guestfs_head_n (g, 10, filename);
  if (lines == NULL)
    return -1;

  if (lines[0] != NULL)
    goto out;

  /* First line is dist release name */
  fs->product_name = safe_strdup (g, lines[0]);

  /* Match SLES first because openSuSE regex overlaps some SLES release strings */
  if (match (g, fs->product_name, re_sles) || match (g, fs->product_name, re_nld)) {
    fs->distro = OS_DISTRO_SLES;

    /* Second line contains version string */
    if (lines[1] == NULL)
      goto out;
    major = match1 (g, lines[1], re_sles_version);
    if (major == NULL)
      goto out;
    fs->major_version = guestfs___parse_unsigned_int (g, major);
    free (major);
    if (fs->major_version == -1)
      goto out;

    /* Third line contains service pack string */
    if (lines[2] == NULL)
      goto out;
    minor = match1 (g, lines[2], re_sles_patchlevel);
    if (minor == NULL)
      goto out;
    fs->minor_version = guestfs___parse_unsigned_int (g, minor);
    free (minor);
    if (fs->minor_version == -1)
      goto out;
  }
  else if (match (g, fs->product_name, re_opensuse)) {
    fs->distro = OS_DISTRO_OPENSUSE;

    /* Second line contains version string */
    if (lines[1] == NULL)
      goto out;
    if (match2 (g, lines[1], re_opensuse_version, &major, &minor)) {
      fs->major_version = guestfs___parse_unsigned_int (g, major);
      fs->minor_version = guestfs___parse_unsigned_int (g, minor);
      free (major);
      free (minor);
      if (fs->major_version == -1 || fs->minor_version == -1)
        goto out;
    }
  }

  r = 0;

out:
  return r;
}