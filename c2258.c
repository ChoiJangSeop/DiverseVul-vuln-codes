copyin_link(struct cpio_file_stat *file_hdr, int in_file_des)
{
  char *link_name = NULL;	/* Name of hard and symbolic links.  */
  int res;			/* Result of various function calls.  */

  if (archive_format != arf_tar && archive_format != arf_ustar)
    {
      if (to_stdout_option)
        {
          tape_toss_input (in_file_des, file_hdr->c_filesize);
          tape_skip_padding (in_file_des, file_hdr->c_filesize);
          return;
        }
      link_name = (char *) xmalloc ((unsigned int) file_hdr->c_filesize + 1);
      link_name[file_hdr->c_filesize] = '\0';
      tape_buffered_read (link_name, in_file_des, file_hdr->c_filesize);
      tape_skip_padding (in_file_des, file_hdr->c_filesize);
    }
  else
    {
      if (to_stdout_option)
        return;
      link_name = xstrdup (file_hdr->c_tar_linkname);
    }

  res = UMASKED_SYMLINK (link_name, file_hdr->c_name,
			 file_hdr->c_mode);
  if (res < 0 && create_dir_flag)
    {
      create_all_directories (file_hdr->c_name);
      res = UMASKED_SYMLINK (link_name, file_hdr->c_name,
			     file_hdr->c_mode);
    }
  if (res < 0)
    {
      error (0, errno, _("%s: Cannot symlink to %s"),
	     quotearg_colon (link_name), quote_n (1, file_hdr->c_name));
      free (link_name);
      return;
    }
  if (!no_chown_flag)
    {
      uid_t uid = set_owner_flag ? set_owner : file_hdr->c_uid;
      gid_t gid = set_group_flag ? set_group : file_hdr->c_gid;
      if ((lchown (file_hdr->c_name, uid, gid) < 0)
  	  && errno != EPERM)
	chown_error_details (file_hdr->c_name, uid, gid);
    }
  free (link_name);
}