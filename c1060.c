grepfile (char const *file, struct stats *stats)
{
  int desc;
  int count;
  int status;

  filename = (file ? file : label ? label : _("(standard input)"));

  if (! file)
    desc = STDIN_FILENO;
  else if (devices == SKIP_DEVICES)
    {
      /* Don't open yet, since that might have side effects on a device.  */
      desc = -1;
    }
  else
    {
      /* When skipping directories, don't worry about directories
         that can't be opened.  */
      desc = open (file, O_RDONLY);
      if (desc < 0 && directories != SKIP_DIRECTORIES)
        {
          suppressible_error (file, errno);
          return 1;
        }
    }

  if (desc < 0
      ? stat (file, &stats->stat) != 0
      : fstat (desc, &stats->stat) != 0)
    {
      suppressible_error (filename, errno);
      if (file)
        close (desc);
      return 1;
    }

  if ((directories == SKIP_DIRECTORIES && S_ISDIR (stats->stat.st_mode))
      || (devices == SKIP_DEVICES && (S_ISCHR (stats->stat.st_mode)
                                      || S_ISBLK (stats->stat.st_mode)
                                      || S_ISSOCK (stats->stat.st_mode)
                                      || S_ISFIFO (stats->stat.st_mode))))
    {
      if (file)
        close (desc);
      return 1;
    }

  /* If there is a regular file on stdout and the current file refers
     to the same i-node, we have to report the problem and skip it.
     Otherwise when matching lines from some other input reach the
     disk before we open this file, we can end up reading and matching
     those lines and appending them to the file from which we're reading.
     Then we'd have what appears to be an infinite loop that'd terminate
     only upon filling the output file system or reaching a quota.
     However, there is no risk of an infinite loop if grep is generating
     no output, i.e., with --silent, --quiet, -q.
     Similarly, with any of these:
       --max-count=N (-m) (for N >= 2)
       --files-with-matches (-l)
       --files-without-match (-L)
     there is no risk of trouble.
     For --max-count=1, grep stops after printing the first match,
     so there is no risk of malfunction.  But even --max-count=2, with
     input==output, while there is no risk of infloop, there is a race
     condition that could result in "alternate" output.  */
  if (!out_quiet && list_files == 0 && 1 < max_count
      && S_ISREG (out_stat.st_mode) && out_stat.st_ino
      && SAME_INODE (stats->stat, out_stat))
    {
      if (! suppress_errors)
        error (0, 0, _("input file %s is also the output"), quote (filename));
      errseen = 1;
      if (file)
        close (desc);
      return 1;
    }

  if (desc < 0)
    {
      desc = open (file, O_RDONLY);
      if (desc < 0)
        {
          suppressible_error (file, errno);
          return 1;
        }
    }

#if defined SET_BINARY
  /* Set input to binary mode.  Pipes are simulated with files
     on DOS, so this includes the case of "foo | grep bar".  */
  if (!isatty (desc))
    SET_BINARY (desc);
#endif

  count = grep (desc, file, stats);
  if (count < 0)
    status = count + 2;
  else
    {
      if (count_matches)
        {
          if (out_file)
            {
              print_filename ();
              if (filename_mask)
                print_sep (SEP_CHAR_SELECTED);
              else
                fputc (0, stdout);
            }
          printf ("%d\n", count);
        }

      status = !count;
      if (list_files == 1 - 2 * status)
        {
          print_filename ();
          fputc ('\n' & filename_mask, stdout);
        }

      if (! file)
        {
          off_t required_offset = outleft ? bufoffset : after_last_match;
          if (required_offset != bufoffset
              && lseek (desc, required_offset, SEEK_SET) < 0
              && S_ISREG (stats->stat.st_mode))
            suppressible_error (filename, errno);
        }
      else
        while (close (desc) != 0)
          if (errno != EINTR)
            {
              suppressible_error (file, errno);
              break;
            }
    }

  return status;
}