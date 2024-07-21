get_link_name (struct cpio_file_stat *file_hdr, int in_file_des)
{
  off_t n = file_hdr->c_filesize + 1;
  char *link_name;
  
  if (n == 0 || n > SIZE_MAX)
    {
      error (0, 0, _("%s: stored filename length too big"), file_hdr->c_name);
      link_name = NULL;
    }
  else
    {
      link_name = xmalloc (n);
      tape_buffered_read (link_name, in_file_des, file_hdr->c_filesize);
      link_name[file_hdr->c_filesize] = '\0';
      tape_skip_padding (in_file_des, file_hdr->c_filesize);
    }
  return link_name;
}