rfbReleaseClientIterator(rfbClientIteratorPtr iterator)
{
  if(iterator->next) rfbDecrClientRef(iterator->next);
  free(iterator);
}