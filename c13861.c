__getcwd_generic (char *buf, size_t size)
{
  /* Lengths of big file name components and entire file names, and a
     deep level of file name nesting.  These numbers are not upper
     bounds; they are merely large values suitable for initial
     allocations, designed to be large enough for most real-world
     uses.  */
  enum
    {
      BIG_FILE_NAME_COMPONENT_LENGTH = 255,
      BIG_FILE_NAME_LENGTH = MIN (4095, PATH_MAX - 1),
      DEEP_NESTING = 100
    };

#if HAVE_OPENAT_SUPPORT
  int fd = AT_FDCWD;
  bool fd_needs_closing = false;
#else
  char dots[DEEP_NESTING * sizeof ".." + BIG_FILE_NAME_COMPONENT_LENGTH + 1];
  char *dotlist = dots;
  size_t dotsize = sizeof dots;
  size_t dotlen = 0;
#endif
  DIR *dirstream = NULL;
  dev_t rootdev, thisdev;
  ino_t rootino, thisino;
  char *dir;
  register char *dirp;
  struct __stat64_t64 st;
  size_t allocated = size;
  size_t used;

#if HAVE_MINIMALLY_WORKING_GETCWD
  /* If AT_FDCWD is not defined, the algorithm below is O(N**2) and
     this is much slower than the system getcwd (at least on
     GNU/Linux).  So trust the system getcwd's results unless they
     look suspicious.

     Use the system getcwd even if we have openat support, since the
     system getcwd works even when a parent is unreadable, while the
     openat-based approach does not.

     But on AIX 5.1..7.1, the system getcwd is not even minimally
     working: If the current directory name is slightly longer than
     PATH_MAX, it omits the first directory component and returns
     this wrong result with errno = 0.  */

# undef getcwd
  dir = getcwd_system (buf, size);
  if (dir || (size && errno == ERANGE))
    return dir;

  /* Solaris getcwd (NULL, 0) fails with errno == EINVAL, but it has
     internal magic that lets it work even if an ancestor directory is
     inaccessible, which is better in many cases.  So in this case try
     again with a buffer that's almost always big enough.  */
  if (errno == EINVAL && buf == NULL && size == 0)
    {
      char big_buffer[BIG_FILE_NAME_LENGTH + 1];
      dir = getcwd_system (big_buffer, sizeof big_buffer);
      if (dir)
        return strdup (dir);
    }

# if HAVE_PARTLY_WORKING_GETCWD
  /* The system getcwd works, except it sometimes fails when it
     shouldn't, setting errno to ERANGE, ENAMETOOLONG, or ENOENT.    */
  if (errno != ERANGE && errno != ENAMETOOLONG && errno != ENOENT)
    return NULL;
# endif
#endif
  if (size == 0)
    {
      if (buf != NULL)
        {
          __set_errno (EINVAL);
          return NULL;
        }

      allocated = BIG_FILE_NAME_LENGTH + 1;
    }

  if (buf == NULL)
    {
      dir = malloc (allocated);
      if (dir == NULL)
        return NULL;
    }
  else
    dir = buf;

  dirp = dir + allocated;
  *--dirp = '\0';

  if (__lstat64_time64 (".", &st) < 0)
    goto lose;
  thisdev = st.st_dev;
  thisino = st.st_ino;

  if (__lstat64_time64 ("/", &st) < 0)
    goto lose;
  rootdev = st.st_dev;
  rootino = st.st_ino;

  while (!(thisdev == rootdev && thisino == rootino))
    {
      struct dirent64 *d;
      dev_t dotdev;
      ino_t dotino;
      bool mount_point;
      int parent_status;
      size_t dirroom;
      size_t namlen;
      bool use_d_ino = true;

      /* Look at the parent directory.  */
#if HAVE_OPENAT_SUPPORT
      fd = __openat64 (fd, "..", O_RDONLY);
      if (fd < 0)
        goto lose;
      fd_needs_closing = true;
      parent_status = __fstat64_time64 (fd, &st);
#else
      dotlist[dotlen++] = '.';
      dotlist[dotlen++] = '.';
      dotlist[dotlen] = '\0';
      parent_status = __lstat64_time64 (dotlist, &st);
#endif
      if (parent_status != 0)
        goto lose;

      if (dirstream && __closedir (dirstream) != 0)
        {
          dirstream = NULL;
          goto lose;
        }

      /* Figure out if this directory is a mount point.  */
      dotdev = st.st_dev;
      dotino = st.st_ino;
      mount_point = dotdev != thisdev;

      /* Search for the last directory.  */
#if HAVE_OPENAT_SUPPORT
      dirstream = __fdopendir (fd);
      if (dirstream == NULL)
        goto lose;
      fd_needs_closing = false;
#else
      dirstream = __opendir (dotlist);
      if (dirstream == NULL)
        goto lose;
      dotlist[dotlen++] = '/';
#endif
      for (;;)
        {
          /* Clear errno to distinguish EOF from error if readdir returns
             NULL.  */
          __set_errno (0);
          d = __readdir64 (dirstream);

          /* When we've iterated through all directory entries without finding
             one with a matching d_ino, rewind the stream and consider each
             name again, but this time, using lstat.  This is necessary in a
             chroot on at least one system (glibc-2.3.6 + linux 2.6.12), where
             .., ../.., ../../.., etc. all had the same device number, yet the
             d_ino values for entries in / did not match those obtained
             via lstat.  */
          if (d == NULL && errno == 0 && use_d_ino)
            {
              use_d_ino = false;
              __rewinddir (dirstream);
              d = __readdir64 (dirstream);
            }

          if (d == NULL)
            {
              if (errno == 0)
                /* EOF on dirstream, which can mean e.g., that the current
                   directory has been removed.  */
                __set_errno (ENOENT);
              goto lose;
            }
          if (d->d_name[0] == '.' &&
              (d->d_name[1] == '\0' ||
               (d->d_name[1] == '.' && d->d_name[2] == '\0')))
            continue;

          if (use_d_ino)
            {
              bool match = (MATCHING_INO (d, thisino) || mount_point);
              if (! match)
                continue;
            }

          {
            int entry_status;
#if HAVE_OPENAT_SUPPORT
            entry_status = __fstatat64_time64 (fd, d->d_name, &st,
					       AT_SYMLINK_NOFOLLOW);
#else
            /* Compute size needed for this file name, or for the file
               name ".." in the same directory, whichever is larger.
               Room for ".." might be needed the next time through
               the outer loop.  */
            size_t name_alloc = _D_ALLOC_NAMLEN (d);
            size_t filesize = dotlen + MAX (sizeof "..", name_alloc);

            if (filesize < dotlen)
              goto memory_exhausted;

            if (dotsize < filesize)
              {
                /* My, what a deep directory tree you have, Grandma.  */
                size_t newsize = MAX (filesize, dotsize * 2);
                size_t i;
                if (newsize < dotsize)
                  goto memory_exhausted;
                if (dotlist != dots)
                  free (dotlist);
                dotlist = malloc (newsize);
                if (dotlist == NULL)
                  goto lose;
                dotsize = newsize;

                i = 0;
                do
                  {
                    dotlist[i++] = '.';
                    dotlist[i++] = '.';
                    dotlist[i++] = '/';
                  }
                while (i < dotlen);
              }

            memcpy (dotlist + dotlen, d->d_name, _D_ALLOC_NAMLEN (d));
            entry_status = __lstat64_time64 (dotlist, &st);
#endif
            /* We don't fail here if we cannot stat() a directory entry.
               This can happen when (network) file systems fail.  If this
               entry is in fact the one we are looking for we will find
               out soon as we reach the end of the directory without
               having found anything.  */
            if (entry_status == 0 && S_ISDIR (st.st_mode)
                && st.st_dev == thisdev && st.st_ino == thisino)
              break;
          }
        }

      dirroom = dirp - dir;
      namlen = _D_EXACT_NAMLEN (d);

      if (dirroom <= namlen)
        {
          if (size != 0)
            {
              __set_errno (ERANGE);
              goto lose;
            }
          else
            {
              char *tmp;
              size_t oldsize = allocated;

              allocated += MAX (allocated, namlen);
              if (allocated < oldsize
                  || ! (tmp = realloc (dir, allocated)))
                goto memory_exhausted;

              /* Move current contents up to the end of the buffer.
                 This is guaranteed to be non-overlapping.  */
              dirp = memcpy (tmp + allocated - (oldsize - dirroom),
                             tmp + dirroom,
                             oldsize - dirroom);
              dir = tmp;
            }
        }
      dirp -= namlen;
      memcpy (dirp, d->d_name, namlen);
      *--dirp = '/';

      thisdev = dotdev;
      thisino = dotino;
    }

  if (dirstream && __closedir (dirstream) != 0)
    {
      dirstream = NULL;
      goto lose;
    }

  if (dirp == &dir[allocated - 1])
    *--dirp = '/';

#if ! HAVE_OPENAT_SUPPORT
  if (dotlist != dots)
    free (dotlist);
#endif

  used = dir + allocated - dirp;
  memmove (dir, dirp, used);

  if (size == 0)
    /* Ensure that the buffer is only as large as necessary.  */
    buf = (used < allocated ? realloc (dir, used) : dir);

  if (buf == NULL)
    /* Either buf was NULL all along, or 'realloc' failed but
       we still have the original string.  */
    buf = dir;

  return buf;

 memory_exhausted:
  __set_errno (ENOMEM);
 lose:
  {
    int save = errno;
    if (dirstream)
      __closedir (dirstream);
#if HAVE_OPENAT_SUPPORT
    if (fd_needs_closing)
       __close_nocancel_nostatus (fd);
#else
    if (dotlist != dots)
      free (dotlist);
#endif
    if (buf == NULL)
      free (dir);
    __set_errno (save);
  }
  return NULL;
}