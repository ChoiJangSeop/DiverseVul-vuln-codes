check_data_region (struct tar_sparse_file *file, size_t i)
{
  off_t size_left;

  if (!lseek_or_error (file, file->stat_info->sparse_map[i].offset))
    return false;
  size_left = file->stat_info->sparse_map[i].numbytes;
  mv_size_left (file->stat_info->archive_file_size - file->dumped_size);

  while (size_left > 0)
    {
      size_t bytes_read;
      size_t rdsize = (size_left > BLOCKSIZE) ? BLOCKSIZE : size_left;
      char diff_buffer[BLOCKSIZE];

      union block *blk = find_next_block ();
      if (!blk)
	{
	  ERROR ((0, 0, _("Unexpected EOF in archive")));
	  return false;
	}
      set_next_block_after (blk);
      bytes_read = safe_read (file->fd, diff_buffer, rdsize);
      if (bytes_read == SAFE_READ_ERROR)
	{
          read_diag_details (file->stat_info->orig_file_name,
			     (file->stat_info->sparse_map[i].offset
			      + file->stat_info->sparse_map[i].numbytes
			      - size_left),
			     rdsize);
	  return false;
	}
      file->dumped_size += bytes_read;
      size_left -= bytes_read;
      mv_size_left (file->stat_info->archive_file_size - file->dumped_size);
      if (memcmp (blk->buffer, diff_buffer, rdsize))
	{
	  report_difference (file->stat_info, _("Contents differ"));
	  return false;
	}
    }
  return true;
}