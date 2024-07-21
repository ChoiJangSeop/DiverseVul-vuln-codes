int main(void) {
#if defined(UMM_INTEGRITY_CHECK)
  TRY(test_integrity_check());
#endif

#if defined(UMM_POISON)
  TRY(test_poison());
#endif

  TRY(random_stress());
  TRY(test_oom_random());

  return 0;
}