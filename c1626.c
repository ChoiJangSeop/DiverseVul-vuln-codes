void json_object_seed(size_t seed) {
    uint32_t new_seed = (uint32_t)seed;

    if (hashtable_seed == 0) {
        if (InterlockedIncrement(&seed_initialized) == 1) {
            /* Do the seeding ourselves */
            if (new_seed == 0)
                new_seed = generate_seed();

            hashtable_seed = new_seed;
        } else {
            /* Wait for another thread to do the seeding */
            do {
                SwitchToThread();
            } while (hashtable_seed == 0);
        }
    }
}