const HeaderEntry* HeaderMapImpl::get(const LowerCaseString& key) const {
  return const_cast<HeaderMapImpl*>(this)->getExisting(key);
}