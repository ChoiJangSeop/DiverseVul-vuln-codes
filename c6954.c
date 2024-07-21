check_sparse_region (struct tar_sparse_file *file, off_t beg, off_t end)
{
  if (!lseek_or_error (file, beg))
    return false;

  while (beg < end)
    {
      size_t bytes_read;
      size_t rdsize = BLOCKSIZE < end - beg ? BLOCKSIZE : end - beg;
      char diff_buffer[BLOCKSIZE];

      bytes_read = safe_read (file->fd, diff_buffer, rdsize);
      if (bytes_read == SAFE_READ_ERROR)
	{
          read_diag_details (file->stat_info->orig_file_name,
	                     beg,
			     rdsize);
	  return false;
	}
      if (!zero_block_p (diff_buffer, bytes_read))
	{
	  char begbuf[INT_BUFSIZE_BOUND (off_t)];
 	  report_difference (file->stat_info,
			     _("File fragment at %s is not a hole"),
			     offtostr (beg, begbuf));
	  return false;
	}

      beg += bytes_read;
    }
  return true;
}