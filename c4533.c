STATIC ptr_t GC_unix_mmap_get_mem(word bytes)
{
    void *result;
    static ptr_t last_addr = HEAP_START;

#   ifndef USE_MMAP_ANON
      static GC_bool initialized = FALSE;

      if (!EXPECT(initialized, TRUE)) {
#       ifdef SYMBIAN
          char* path = GC_get_private_path_and_zero_file();
          zero_fd = open(path, O_RDWR | O_CREAT, 0666);
          free(path);
#       else
          zero_fd = open("/dev/zero", O_RDONLY);
#       endif
          if (zero_fd == -1)
            ABORT("Could not open /dev/zero");
          if (fcntl(zero_fd, F_SETFD, FD_CLOEXEC) == -1)
            WARN("Could not set FD_CLOEXEC for /dev/zero\n", 0);

          initialized = TRUE;
      }
#   endif

    if (bytes & (GC_page_size - 1)) ABORT("Bad GET_MEM arg");
    result = mmap(last_addr, bytes, (PROT_READ | PROT_WRITE)
                                    | (GC_pages_executable ? PROT_EXEC : 0),
                  GC_MMAP_FLAGS | OPT_MAP_ANON, zero_fd, 0/* offset */);
#   undef IGNORE_PAGES_EXECUTABLE

    if (result == MAP_FAILED) return(0);
    last_addr = (ptr_t)ROUNDUP_PAGESIZE((word)result + bytes);
#   if !defined(LINUX)
      if (last_addr == 0) {
        /* Oops.  We got the end of the address space.  This isn't      */
        /* usable by arbitrary C code, since one-past-end pointers      */
        /* don't work, so we discard it and try again.                  */
        munmap(result, (size_t)(-GC_page_size) - (size_t)result);
                        /* Leave last page mapped, so we can't repeat.  */
        return GC_unix_mmap_get_mem(bytes);
      }
#   else
      GC_ASSERT(last_addr != 0);
#   endif
    if (((word)result % HBLKSIZE) != 0)
      ABORT(
       "GC_unix_get_mem: Memory returned by mmap is not aligned to HBLKSIZE.");
    return((ptr_t)result);
}