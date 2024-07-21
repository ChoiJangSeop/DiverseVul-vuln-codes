catch_exception_raise(mach_port_t exception_port GC_ATTR_UNUSED,
                      mach_port_t thread, mach_port_t task GC_ATTR_UNUSED,
                      exception_type_t exception, exception_data_t code,
                      mach_msg_type_number_t code_count GC_ATTR_UNUSED)
{
  kern_return_t r;
  char *addr;
  struct hblk *h;
  unsigned int i;
  thread_state_flavor_t flavor = DARWIN_EXC_STATE;
  mach_msg_type_number_t exc_state_count = DARWIN_EXC_STATE_COUNT;
  DARWIN_EXC_STATE_T exc_state;

  if (exception != EXC_BAD_ACCESS || code[0] != KERN_PROTECTION_FAILURE) {
#   ifdef DEBUG_EXCEPTION_HANDLING
      /* We aren't interested, pass it on to the old handler */
      GC_log_printf("Exception: 0x%x Code: 0x%x 0x%x in catch...\n",
                    exception, code_count > 0 ? code[0] : -1,
                    code_count > 1 ? code[1] : -1);
#   endif
    return FWD();
  }

  r = thread_get_state(thread, flavor, (natural_t*)&exc_state,
                       &exc_state_count);
  if(r != KERN_SUCCESS) {
    /* The thread is supposed to be suspended while the exception       */
    /* handler is called.  This shouldn't fail.                         */
#   ifdef BROKEN_EXCEPTION_HANDLING
      GC_err_printf("thread_get_state failed in catch_exception_raise\n");
      return KERN_SUCCESS;
#   else
      ABORT("thread_get_state failed in catch_exception_raise");
#   endif
  }

  /* This is the address that caused the fault */
  addr = (char*) exc_state.DARWIN_EXC_STATE_DAR;
  if (HDR(addr) == 0) {
    /* Ugh... just like the SIGBUS problem above, it seems we get       */
    /* a bogus KERN_PROTECTION_FAILURE every once and a while.  We wait */
    /* till we get a bunch in a row before doing anything about it.     */
    /* If a "real" fault ever occurs it'll just keep faulting over and  */
    /* over and we'll hit the limit pretty quickly.                     */
#   ifdef BROKEN_EXCEPTION_HANDLING
      static char *last_fault;
      static int last_fault_count;

      if(addr != last_fault) {
        last_fault = addr;
        last_fault_count = 0;
      }
      if(++last_fault_count < 32) {
        if(last_fault_count == 1)
          WARN("Ignoring KERN_PROTECTION_FAILURE at %p\n", addr);
        return KERN_SUCCESS;
      }

      GC_err_printf(
        "Unexpected KERN_PROTECTION_FAILURE at %p; aborting...\n", addr);
      /* Can't pass it along to the signal handler because that is      */
      /* ignoring SIGBUS signals.  We also shouldn't call ABORT here as */
      /* signals don't always work too well from the exception handler. */
      EXIT();
#   else /* BROKEN_EXCEPTION_HANDLING */
      /* Pass it along to the next exception handler
         (which should call SIGBUS/SIGSEGV) */
      return FWD();
#   endif /* !BROKEN_EXCEPTION_HANDLING */
  }

# ifdef BROKEN_EXCEPTION_HANDLING
    /* Reset the number of consecutive SIGBUSs */
    GC_sigbus_count = 0;
# endif

  if (GC_mprotect_state == GC_MP_NORMAL) { /* common case */
    h = (struct hblk*)((word)addr & ~(GC_page_size-1));
    UNPROTECT(h, GC_page_size);
    for (i = 0; i < divHBLKSZ(GC_page_size); i++) {
      register int index = PHT_HASH(h+i);
      async_set_pht_entry_from_index(GC_dirty_pages, index);
    }
  } else if (GC_mprotect_state == GC_MP_DISCARDING) {
    /* Lie to the thread for now. No sense UNPROTECT()ing the memory
       when we're just going to PROTECT() it again later. The thread
       will just fault again once it resumes */
  } else {
    /* Shouldn't happen, i don't think */
    GC_err_printf("KERN_PROTECTION_FAILURE while world is stopped\n");
    return FWD();
  }
  return KERN_SUCCESS;
}