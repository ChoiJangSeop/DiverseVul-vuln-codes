realpath_stk (const char *name, char *resolved,
              struct scratch_buffer *rname_buf)
{
  char *dest;
  char const *start;
  char const *end;
  int num_links = 0;

  if (name == NULL)
    {
      /* As per Single Unix Specification V2 we must return an error if
         either parameter is a null pointer.  We extend this to allow
         the RESOLVED parameter to be NULL in case the we are expected to
         allocate the room for the return value.  */
      __set_errno (EINVAL);
      return NULL;
    }

  if (name[0] == '\0')
    {
      /* As per Single Unix Specification V2 we must return an error if
         the name argument points to an empty string.  */
      __set_errno (ENOENT);
      return NULL;
    }

  struct scratch_buffer extra_buffer, link_buffer;
  scratch_buffer_init (&extra_buffer);
  scratch_buffer_init (&link_buffer);
  scratch_buffer_init (rname_buf);
  char *rname_on_stack = rname_buf->data;
  char *rname = rname_on_stack;
  bool end_in_extra_buffer = false;
  bool failed = true;

  /* This is always zero for Posix hosts, but can be 2 for MS-Windows
     and MS-DOS X:/foo/bar file names.  */
  idx_t prefix_len = FILE_SYSTEM_PREFIX_LEN (name);

  if (!IS_ABSOLUTE_FILE_NAME (name))
    {
      while (!__getcwd (rname, rname_buf->length))
        {
          if (errno != ERANGE)
            {
              dest = rname;
              goto error;
            }
          if (!scratch_buffer_grow (rname_buf))
            goto error_nomem;
          rname = rname_buf->data;
        }
      dest = __rawmemchr (rname, '\0');
      start = name;
      prefix_len = FILE_SYSTEM_PREFIX_LEN (rname);
    }
  else
    {
      dest = __mempcpy (rname, name, prefix_len);
      *dest++ = '/';
      if (DOUBLE_SLASH_IS_DISTINCT_ROOT)
        {
          if (prefix_len == 0 /* implies ISSLASH (name[0]) */
              && ISSLASH (name[1]) && !ISSLASH (name[2]))
            *dest++ = '/';
          *dest = '\0';
        }
      start = name + prefix_len;
    }

  for ( ; *start; start = end)
    {
      /* Skip sequence of multiple file name separators.  */
      while (ISSLASH (*start))
        ++start;

      /* Find end of component.  */
      for (end = start; *end && !ISSLASH (*end); ++end)
        /* Nothing.  */;

      /* Length of this file name component; it can be zero if a file
         name ends in '/'.  */
      idx_t startlen = end - start;

      if (startlen == 0)
        break;
      else if (startlen == 1 && start[0] == '.')
        /* nothing */;
      else if (startlen == 2 && start[0] == '.' && start[1] == '.')
        {
          /* Back up to previous component, ignore if at root already.  */
          if (dest > rname + prefix_len + 1)
            for (--dest; dest > rname && !ISSLASH (dest[-1]); --dest)
              continue;
          if (DOUBLE_SLASH_IS_DISTINCT_ROOT
              && dest == rname + 1 && !prefix_len
              && ISSLASH (*dest) && !ISSLASH (dest[1]))
            dest++;
        }
      else
        {
          if (!ISSLASH (dest[-1]))
            *dest++ = '/';

          while (rname + rname_buf->length - dest
                 < startlen + sizeof dir_suffix)
            {
              idx_t dest_offset = dest - rname;
              if (!scratch_buffer_grow_preserve (rname_buf))
                goto error_nomem;
              rname = rname_buf->data;
              dest = rname + dest_offset;
            }

          dest = __mempcpy (dest, start, startlen);
          *dest = '\0';

          char *buf;
          ssize_t n;
          while (true)
            {
              buf = link_buffer.data;
              idx_t bufsize = link_buffer.length;
              n = __readlink (rname, buf, bufsize - 1);
              if (n < bufsize - 1)
                break;
              if (!scratch_buffer_grow (&link_buffer))
                goto error_nomem;
            }
          if (0 <= n)
            {
              if (++num_links > __eloop_threshold ())
                {
                  __set_errno (ELOOP);
                  goto error;
                }

              buf[n] = '\0';

              char *extra_buf = extra_buffer.data;
              idx_t end_idx IF_LINT (= 0);
              if (end_in_extra_buffer)
                end_idx = end - extra_buf;
              size_t len = strlen (end);
              if (INT_ADD_OVERFLOW (len, n))
                {
                  __set_errno (ENOMEM);
                  goto error_nomem;
                }
              while (extra_buffer.length <= len + n)
                {
                  if (!scratch_buffer_grow_preserve (&extra_buffer))
                    goto error_nomem;
                  extra_buf = extra_buffer.data;
                }
              if (end_in_extra_buffer)
                end = extra_buf + end_idx;

              /* Careful here, end may be a pointer into extra_buf... */
              memmove (&extra_buf[n], end, len + 1);
              name = end = memcpy (extra_buf, buf, n);
              end_in_extra_buffer = true;

              if (IS_ABSOLUTE_FILE_NAME (buf))
                {
                  idx_t pfxlen = FILE_SYSTEM_PREFIX_LEN (buf);

                  dest = __mempcpy (rname, buf, pfxlen);
                  *dest++ = '/'; /* It's an absolute symlink */
                  if (DOUBLE_SLASH_IS_DISTINCT_ROOT)
                    {
                      if (ISSLASH (buf[1]) && !ISSLASH (buf[2]) && !pfxlen)
                        *dest++ = '/';
                      *dest = '\0';
                    }
                  /* Install the new prefix to be in effect hereafter.  */
                  prefix_len = pfxlen;
                }
              else
                {
                  /* Back up to previous component, ignore if at root
                     already: */
                  if (dest > rname + prefix_len + 1)
                    for (--dest; dest > rname && !ISSLASH (dest[-1]); --dest)
                      continue;
                  if (DOUBLE_SLASH_IS_DISTINCT_ROOT && dest == rname + 1
                      && ISSLASH (*dest) && !ISSLASH (dest[1]) && !prefix_len)
                    dest++;
                }
            }
          else if (! (suffix_requires_dir_check (end)
                      ? dir_check (rname, dest)
                      : errno == EINVAL))
            goto error;
        }
    }
  if (dest > rname + prefix_len + 1 && ISSLASH (dest[-1]))
    --dest;
  if (DOUBLE_SLASH_IS_DISTINCT_ROOT && dest == rname + 1 && !prefix_len
      && ISSLASH (*dest) && !ISSLASH (dest[1]))
    dest++;
  failed = false;

error:
  *dest++ = '\0';
  if (resolved != NULL && dest - rname <= get_path_max ())
    rname = strcpy (resolved, rname);

error_nomem:
  scratch_buffer_free (&extra_buffer);
  scratch_buffer_free (&link_buffer);

  if (failed || rname == resolved)
    {
      scratch_buffer_free (rname_buf);
      return failed ? NULL : resolved;
    }

  return scratch_buffer_dupfree (rname_buf, dest - rname);
}