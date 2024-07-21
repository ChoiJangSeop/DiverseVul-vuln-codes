void json_object_seed(size_t seed) {
    uint32_t new_seed = (uint32_t)seed;

    if (hashtable_seed == 0) {
        if (new_seed == 0) {
            /* Explicit synchronization fences are not supported by the
               __sync builtins, so every thread getting here has to
               generate the seed value.
            */
            new_seed = generate_seed();
        }

        do {
            if (__sync_bool_compare_and_swap(&hashtable_seed, 0, new_seed)) {
                /* We were the first to seed */
                break;
            } else {
                /* Wait for another thread to do the seeding */
#ifdef HAVE_SCHED_YIELD
                sched_yield();
#endif
            }
        } while(hashtable_seed == 0);
    }
}