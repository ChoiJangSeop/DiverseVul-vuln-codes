sraSpanInsertBefore(sraSpan *newspan, sraSpan *before) {
  newspan->_next = before;
  newspan->_prev = before->_prev;
  before->_prev->_next = newspan;
  before->_prev = newspan;
}