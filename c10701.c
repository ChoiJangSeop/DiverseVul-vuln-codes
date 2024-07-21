process_copy_in ()
{
  FILE *tty_in = NULL;		/* Interactive file for rename option.  */
  FILE *tty_out = NULL;		/* Interactive file for rename option.  */
  FILE *rename_in = NULL;	/* Batch file for rename option.  */
  struct stat file_stat;	/* Output file stat record.  */
  struct cpio_file_stat file_hdr = CPIO_FILE_STAT_INITIALIZER;
                                /* Output header information.  */
  int in_file_des;		/* Input file descriptor.  */
  char skip_file;		/* Flag for use with patterns.  */
  int i;			/* Loop index variable.  */

  newdir_umask = umask (0);     /* Reset umask to preserve modes of
				   created files  */
  
  /* Initialize the copy in.  */
  if (pattern_file_name)
    {
      read_pattern_file ();
    }
  
  if (rename_batch_file)
    {
      rename_in = fopen (rename_batch_file, "r");
      if (rename_in == NULL)
	{
	  error (PAXEXIT_FAILURE, errno, TTY_NAME);
	}
    }
  else if (rename_flag)
    {
      /* Open interactive file pair for rename operation.  */
      tty_in = fopen (TTY_NAME, "r");
      if (tty_in == NULL)
	{
	  error (PAXEXIT_FAILURE, errno, TTY_NAME);
	}
      tty_out = fopen (TTY_NAME, "w");
      if (tty_out == NULL)
	{
	  error (PAXEXIT_FAILURE, errno, TTY_NAME);
	}
    }

  /* Get date and time if needed for processing the table option.  */
  if (table_flag && verbose_flag)
    {
      time (&current_time);
    }

  /* Check whether the input file might be a tape.  */
  in_file_des = archive_des;
  if (_isrmt (in_file_des))
    {
      input_is_special = 1;
      input_is_seekable = 0;
    }
  else
    {
      if (fstat (in_file_des, &file_stat))
	error (PAXEXIT_FAILURE, errno, _("standard input is closed"));
      input_is_special =
#ifdef S_ISBLK
	S_ISBLK (file_stat.st_mode) ||
#endif
	S_ISCHR (file_stat.st_mode);
      input_is_seekable = S_ISREG (file_stat.st_mode);
    }
  output_is_seekable = true;

  change_dir ();
  
  /* While there is more input in the collection, process the input.  */
  while (1)
    {
      swapping_halfwords = swapping_bytes = false;

      /* Start processing the next file by reading the header.  */
      read_in_header (&file_hdr, in_file_des);

#ifdef DEBUG_CPIO
      if (debug_flag)
	{
	  struct cpio_file_stat *h;
	  h = &file_hdr;
	  fprintf (stderr, 
		"magic = 0%o, ino = %ld, mode = 0%o, uid = %d, gid = %d\n",
		h->c_magic, (long)h->c_ino, h->c_mode, h->c_uid, h->c_gid);
	  fprintf (stderr, 
		"nlink = %d, mtime = %d, filesize = %d, dev_maj = 0x%x\n",
		h->c_nlink, h->c_mtime, h->c_filesize, h->c_dev_maj);
	  fprintf (stderr, 
	        "dev_min = 0x%x, rdev_maj = 0x%x, rdev_min = 0x%x, namesize = %d\n",
		h->c_dev_min, h->c_rdev_maj, h->c_rdev_min, h->c_namesize);
	  fprintf (stderr, 
		"chksum = %d, name = \"%s\", tar_linkname = \"%s\"\n",
		h->c_chksum, h->c_name, 
		h->c_tar_linkname ? h->c_tar_linkname : "(null)" );

	}
#endif
      if (file_hdr.c_namesize == 0)
	skip_file = true;
      else
	{
	  /* Is this the header for the TRAILER file?  */
	  if (strcmp (CPIO_TRAILER_NAME, file_hdr.c_name) == 0)
	    break;

	  cpio_safer_name_suffix (file_hdr.c_name, false, !no_abs_paths_flag,
				  false);
      
	  /* Does the file name match one of the given patterns?  */
	  if (num_patterns <= 0)
	    skip_file = false;
	  else
	    {
	      skip_file = copy_matching_files;
	      for (i = 0; i < num_patterns
		     && skip_file == copy_matching_files; i++)
		{
		  if (fnmatch (save_patterns[i], file_hdr.c_name, 0) == 0)
		    skip_file = !copy_matching_files;
		}
	    }
	}
      
      if (skip_file)
	{
	  /* If we're skipping a file with links, there might be other
	     links that we didn't skip, and this file might have the
	     data for the links.  If it does, we'll copy in the data
	     to the links, but not to this file.  */
	  if (file_hdr.c_nlink > 1 && (archive_format == arf_newascii
	      || archive_format == arf_crcascii) )
	    {
	      if (create_defered_links_to_skipped(&file_hdr, in_file_des) < 0)
	        {
		  tape_toss_input (in_file_des, file_hdr.c_filesize);
		  tape_skip_padding (in_file_des, file_hdr.c_filesize);
		}
	    }
	  else
	    {
	      tape_toss_input (in_file_des, file_hdr.c_filesize);
	      tape_skip_padding (in_file_des, file_hdr.c_filesize);
	    }
	}
      else if (table_flag)
	{
	  list_file (&file_hdr, in_file_des);
	}
      else if (append_flag)
	{
	  tape_toss_input (in_file_des, file_hdr.c_filesize);
	  tape_skip_padding (in_file_des, file_hdr.c_filesize);
	}
      else if (only_verify_crc_flag)
	{
#ifdef CP_IFLNK
	  if ((file_hdr.c_mode & CP_IFMT) == CP_IFLNK)
	    {
	      if (archive_format != arf_tar && archive_format != arf_ustar)
		{
		  tape_toss_input (in_file_des, file_hdr.c_filesize);
		  tape_skip_padding (in_file_des, file_hdr.c_filesize);
		  continue;
		}
	    }
#endif
	    crc = 0;
	    tape_toss_input (in_file_des, file_hdr.c_filesize);
	    tape_skip_padding (in_file_des, file_hdr.c_filesize);
	    if (crc != file_hdr.c_chksum)
	      {
		error (0, 0, _("%s: checksum error (0x%x, should be 0x%x)"),
		       file_hdr.c_name, crc, file_hdr.c_chksum);
	      }
         /* Debian hack: -v and -V now work with --only-verify-crc.
            (99/11/10) -BEM */
	    if (verbose_flag)
	      {
		fprintf (stderr, "%s\n", file_hdr.c_name);
	      }
	    if (dot_flag)
	      {
		fputc ('.', stderr);
	      }
	}
      else
	{
	  /* Copy the input file into the directory structure.  */

	  /* Do we need to rename the file? */
	  if (rename_flag || rename_batch_file)
	    {
	      if (query_rename(&file_hdr, tty_in, tty_out, rename_in) < 0)
	        {
		  tape_toss_input (in_file_des, file_hdr.c_filesize);
		  tape_skip_padding (in_file_des, file_hdr.c_filesize);
		  continue;
		}
	    }

	  copyin_file(&file_hdr, in_file_des);

	  if (verbose_flag)
	    fprintf (stderr, "%s\n", file_hdr.c_name);
	  if (dot_flag)
	    fputc ('.', stderr);
	}
    }

  if (dot_flag)
    fputc ('\n', stderr);

  apply_delayed_set_stat ();

  cpio_file_stat_free (&file_hdr);
  
  if (append_flag)
    return;

  if (archive_format == arf_newascii || archive_format == arf_crcascii)
    {
      create_final_defers ();
    }
  if (!quiet_flag)
    {
      size_t blocks;
      blocks = (input_bytes + io_block_size - 1) / io_block_size;
      fprintf (stderr,
	       ngettext ("%lu block\n", "%lu blocks\n",
			 (unsigned long) blocks),
	       (unsigned long) blocks);
    }
}