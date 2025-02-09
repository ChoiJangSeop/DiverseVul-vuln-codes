rfbClientIteratorNext(rfbClientIteratorPtr i)
{
  if(i->next == 0) {
    LOCK(rfbClientListMutex);
    i->next = i->screen->clientHead;
    UNLOCK(rfbClientListMutex);
  } else {
    rfbClientPtr cl = i->next;
    i->next = i->next->next;
    rfbDecrClientRef(cl);
  }

#if defined(LIBVNCSERVER_HAVE_LIBPTHREAD) || defined(LIBVNCSERVER_HAVE_WIN32THREADS)
    if(!i->closedToo)
      while(i->next && i->next->sock<0)
        i->next = i->next->next;
    if(i->next)
      rfbIncrClientRef(i->next);
#endif

    return i->next;
}