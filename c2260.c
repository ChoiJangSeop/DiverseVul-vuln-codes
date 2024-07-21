get_link_name (struct cpio_file_stat *file_hdr, int in_file_des)
{
  char *link_name;
  
  if (file_hdr->c_filesize < 0 || file_hdr->c_filesize > SIZE_MAX-1)
    {
      error (0, 0, _("%s: stored filename length is out of range"),
	     file_hdr->c_name);
      link_name = NULL;
    }
  else
    {
      link_name = xmalloc (file_hdr->c_filesize);
      tape_buffered_read (link_name, in_file_des, file_hdr->c_filesize);
      link_name[file_hdr->c_filesize] = '\0';
      tape_skip_padding (in_file_des, file_hdr->c_filesize);
    }
  return link_name;
}