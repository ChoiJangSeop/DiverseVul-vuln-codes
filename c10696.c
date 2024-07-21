process_copy_out ()
{
  dynamic_string input_name;	/* Name of file read from stdin.  */
  struct stat file_stat;	/* Stat record for file.  */
  struct cpio_file_stat file_hdr = CPIO_FILE_STAT_INITIALIZER;
                                /* Output header information.  */
  int in_file_des;		/* Source file descriptor.  */
  int out_file_des;		/* Output file descriptor.  */
  char *orig_file_name = NULL;

  /* Initialize the copy out.  */
  ds_init (&input_name, 128);
  file_hdr.c_magic = 070707;

  /* Check whether the output file might be a tape.  */
  out_file_des = archive_des;
  if (_isrmt (out_file_des))
    {
      output_is_special = 1;
      output_is_seekable = 0;
    }
  else
    {
      if (fstat (out_file_des, &file_stat))
	error (PAXEXIT_FAILURE, errno, _("standard output is closed"));
      output_is_special =
#ifdef S_ISBLK
	S_ISBLK (file_stat.st_mode) ||
#endif
	S_ISCHR (file_stat.st_mode);
      output_is_seekable = S_ISREG (file_stat.st_mode);
    }

  change_dir ();
  
  if (append_flag)
    {
      process_copy_in ();
      prepare_append (out_file_des);
    }

  /* Copy files with names read from stdin.  */
  while (ds_fgetstr (stdin, &input_name, name_end) != NULL)
    {
      /* Check for blank line.  */
      if (input_name.ds_string[0] == 0)
	{
	  error (0, 0, _("blank line ignored"));
	  continue;
	}

      /* Process next file.  */
      if ((*xstat) (input_name.ds_string, &file_stat) < 0)
	stat_error (input_name.ds_string);
      else
	{
	  /* Set values in output header.  */
	  stat_to_cpio (&file_hdr, &file_stat);
	  
	  if (archive_format == arf_tar || archive_format == arf_ustar)
	    {
	      if (file_hdr.c_mode & CP_IFDIR)
		{
		  int len = strlen (input_name.ds_string);
		  /* Make sure the name ends with a slash */
		  if (input_name.ds_string[len-1] != '/')
		    {
		      ds_resize (&input_name, len + 2);
		      input_name.ds_string[len] = '/';
		      input_name.ds_string[len+1] = 0;
		    }
		}
	    }
	  
	  assign_string (&orig_file_name, input_name.ds_string);
	  cpio_safer_name_suffix (input_name.ds_string, false,
				  !no_abs_paths_flag, true);
	  cpio_set_c_name (&file_hdr, input_name.ds_string);

	  /* Copy the named file to the output.  */
	  switch (file_hdr.c_mode & CP_IFMT)
	    {
	    case CP_IFREG:
	      if (archive_format == arf_tar || archive_format == arf_ustar)
		{
		  char *otherfile;
		  if ((otherfile = find_inode_file (file_hdr.c_ino,
						    file_hdr.c_dev_maj,
						    file_hdr.c_dev_min)))
		    {
		      file_hdr.c_tar_linkname = otherfile;
		      if (write_out_header (&file_hdr, out_file_des))
			continue;
		      break;
		    }
		}
	      if ( (archive_format == arf_newascii || archive_format == arf_crcascii)
		  && (file_hdr.c_nlink > 1) )
		{
		  if (last_link (&file_hdr) )
		    {
		      writeout_other_defers (&file_hdr, out_file_des);
		    }
		  else
		    {
		      add_link_defer (&file_hdr);
		      break;
		    }
		}
	      in_file_des = open (orig_file_name,
				  O_RDONLY | O_BINARY, 0);
	      if (in_file_des < 0)
		{
		  open_error (orig_file_name);
		  continue;
		}

	      if (archive_format == arf_crcascii)
		file_hdr.c_chksum = read_for_checksum (in_file_des,
						       file_hdr.c_filesize,
						       orig_file_name);

	      if (write_out_header (&file_hdr, out_file_des))
		continue;
	      copy_files_disk_to_tape (in_file_des,
				       out_file_des, file_hdr.c_filesize,
				       orig_file_name);
	      warn_if_file_changed(orig_file_name, file_hdr.c_filesize,
                                   file_hdr.c_mtime);

	      if (archive_format == arf_tar || archive_format == arf_ustar)
		add_inode (file_hdr.c_ino, orig_file_name, file_hdr.c_dev_maj,
			   file_hdr.c_dev_min);

	      tape_pad_output (out_file_des, file_hdr.c_filesize);

	      if (reset_time_flag)
                set_file_times (in_file_des,
				orig_file_name,
                                file_stat.st_atime, file_stat.st_mtime);
	      if (close (in_file_des) < 0)
		close_error (orig_file_name);
	      break;

	    case CP_IFDIR:
	      file_hdr.c_filesize = 0;
	      if (write_out_header (&file_hdr, out_file_des))
		continue;
	      break;

	    case CP_IFCHR:
	    case CP_IFBLK:
#ifdef CP_IFSOCK
	    case CP_IFSOCK:
#endif
#ifdef CP_IFIFO
	    case CP_IFIFO:
#endif
	      if (archive_format == arf_tar)
		{
		  error (0, 0, _("%s not dumped: not a regular file"),
			 orig_file_name);
		  continue;
		}
	      else if (archive_format == arf_ustar)
		{
		  char *otherfile;
		  if ((otherfile = find_inode_file (file_hdr.c_ino,
						    file_hdr.c_dev_maj,
						    file_hdr.c_dev_min)))
		    {
		      /* This file is linked to another file already in the 
		         archive, so write it out as a hard link. */
		      file_hdr.c_mode = (file_stat.st_mode & 07777);
		      file_hdr.c_mode |= CP_IFREG;
		      file_hdr.c_tar_linkname = otherfile;
		      if (write_out_header (&file_hdr, out_file_des))
			continue;
		      break;
		    }
		  add_inode (file_hdr.c_ino, orig_file_name, 
			     file_hdr.c_dev_maj, file_hdr.c_dev_min);
		}
	      file_hdr.c_filesize = 0;
	      if (write_out_header (&file_hdr, out_file_des))
		continue;
	      break;

#ifdef CP_IFLNK
	    case CP_IFLNK:
	      {
		char *link_name = (char *) xmalloc (file_stat.st_size + 1);
		int link_size;

		link_size = readlink (orig_file_name, link_name,
			              file_stat.st_size);
		if (link_size < 0)
		  {
		    readlink_warn (orig_file_name);
		    free (link_name);
		    continue;
		  }
		link_name[link_size] = 0;
		cpio_safer_name_suffix (link_name, false,
					!no_abs_paths_flag, true);
		link_size = strlen (link_name);
		file_hdr.c_filesize = link_size;
		if (archive_format == arf_tar || archive_format == arf_ustar)
		  {
		    if (link_size + 1 > 100)
		      {
			error (0, 0, _("%s: symbolic link too long"),
			       file_hdr.c_name);
		      }
		    else
		      {
			link_name[link_size] = '\0';
			file_hdr.c_tar_linkname = link_name;
			if (write_out_header (&file_hdr, out_file_des))
			  continue;
		      }
		  }
		else
		  {
		    if (write_out_header (&file_hdr, out_file_des))
		      continue;
		    tape_buffered_write (link_name, out_file_des, link_size);
		    tape_pad_output (out_file_des, link_size);
		  }
		free (link_name);
	      }
	      break;
#endif

	    default:
	      error (0, 0, _("%s: unknown file type"), orig_file_name);
	    }
	  
	  if (verbose_flag)
	    fprintf (stderr, "%s\n", orig_file_name);
	  if (dot_flag)
	    fputc ('.', stderr);
	}
    }

  free (orig_file_name);
  
  writeout_final_defers(out_file_des);
  /* The collection is complete; append the trailer.  */
  file_hdr.c_ino = 0;
  file_hdr.c_mode = 0;
  file_hdr.c_uid = 0;
  file_hdr.c_gid = 0;
  file_hdr.c_nlink = 1;		/* Must be 1 for crc format.  */
  file_hdr.c_dev_maj = 0;
  file_hdr.c_dev_min = 0;
  file_hdr.c_rdev_maj = 0;
  file_hdr.c_rdev_min = 0;
  file_hdr.c_mtime = 0;
  file_hdr.c_chksum = 0;

  file_hdr.c_filesize = 0;
  cpio_set_c_name (&file_hdr, CPIO_TRAILER_NAME);
  if (archive_format != arf_tar && archive_format != arf_ustar)
    write_out_header (&file_hdr, out_file_des);
  else
    write_nuls_to_file (1024, out_file_des, tape_buffered_write);

  /* Fill up the output block.  */
  tape_clear_rest_of_block (out_file_des);
  tape_empty_output_buffer (out_file_des);
  if (dot_flag)
    fputc ('\n', stderr);
  if (!quiet_flag)
    {
      size_t blocks = (output_bytes + io_block_size - 1) / io_block_size;
      fprintf (stderr,
	       ngettext ("%lu block\n", "%lu blocks\n",
			 (unsigned long) blocks), (unsigned long) blocks);
    }
  cpio_file_stat_free (&file_hdr);
}