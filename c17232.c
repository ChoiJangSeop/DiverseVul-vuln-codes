int llhttp__internal__c_test_flags_5(
    llhttp__internal_t* state,
    const unsigned char* p,
    const unsigned char* endp) {
  return (state->flags & 32) == 32;
}