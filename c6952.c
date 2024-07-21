sparse_extract_region (struct tar_sparse_file *file, size_t i)
{
  off_t write_size;

  if (!lseek_or_error (file, file->stat_info->sparse_map[i].offset))
    return false;

  write_size = file->stat_info->sparse_map[i].numbytes;

  if (write_size == 0)
    {
      /* Last block of the file is a hole */
      if (file->seekable && sys_truncate (file->fd))
	truncate_warn (file->stat_info->orig_file_name);
    }
  else while (write_size > 0)
    {
      size_t count;
      size_t wrbytes = (write_size > BLOCKSIZE) ? BLOCKSIZE : write_size;
      union block *blk = find_next_block ();
      if (!blk)
	{
	  ERROR ((0, 0, _("Unexpected EOF in archive")));
	  return false;
	}
      set_next_block_after (blk);
      count = blocking_write (file->fd, blk->buffer, wrbytes);
      write_size -= count;
      file->dumped_size += count;
      mv_size_left (file->stat_info->archive_file_size - file->dumped_size);
      file->offset += count;
      if (count != wrbytes)
	{
	  write_error_details (file->stat_info->orig_file_name,
			       count, wrbytes);
	  return false;
	}
    }
  return true;
}