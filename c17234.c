int llhttp__internal__c_test_lenient_flags_2(
    llhttp__internal_t* state,
    const unsigned char* p,
    const unsigned char* endp) {
  return (state->lenient_flags & 1) == 1;
}