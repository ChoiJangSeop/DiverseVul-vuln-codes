sraSpanInsertAfter(sraSpan *newspan, sraSpan *after) {
  newspan->_next = after->_next;
  newspan->_prev = after;
  after->_next->_prev = newspan;
  after->_next = newspan;
}