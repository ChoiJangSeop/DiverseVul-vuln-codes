STATIC ptr_t GC_unix_sbrk_get_mem(word bytes)
{
  ptr_t result;
# ifdef IRIX5
    /* Bare sbrk isn't thread safe.  Play by malloc rules.      */
    /* The equivalent may be needed on other systems as well.   */
    __LOCK_MALLOC();
# endif
  {
    ptr_t cur_brk = (ptr_t)sbrk(0);
    SBRK_ARG_T lsbs = (word)cur_brk & (GC_page_size-1);

    if ((SBRK_ARG_T)bytes < 0) {
        result = 0; /* too big */
        goto out;
    }
    if (lsbs != 0) {
        if((ptr_t)sbrk(GC_page_size - lsbs) == (ptr_t)(-1)) {
            result = 0;
            goto out;
        }
    }
#   ifdef ADD_HEAP_GUARD_PAGES
      /* This is useful for catching severe memory overwrite problems that */
      /* span heap sections.  It shouldn't otherwise be turned on.         */
      {
        ptr_t guard = (ptr_t)sbrk((SBRK_ARG_T)GC_page_size);
        if (mprotect(guard, GC_page_size, PROT_NONE) != 0)
            ABORT("ADD_HEAP_GUARD_PAGES: mprotect failed");
      }
#   endif /* ADD_HEAP_GUARD_PAGES */
    result = (ptr_t)sbrk((SBRK_ARG_T)bytes);
    if (result == (ptr_t)(-1)) result = 0;
  }
 out:
# ifdef IRIX5
    __UNLOCK_MALLOC();
# endif
  return(result);
}