int RAND_DRBG_generate(RAND_DRBG *drbg, unsigned char *out, size_t outlen,
                       int prediction_resistance,
                       const unsigned char *adin, size_t adinlen)
{
    int reseed_required = 0;

    if (drbg->state != DRBG_READY) {
        /* try to recover from previous errors */
        rand_drbg_restart(drbg, NULL, 0, 0);

        if (drbg->state == DRBG_ERROR) {
            RANDerr(RAND_F_RAND_DRBG_GENERATE, RAND_R_IN_ERROR_STATE);
            return 0;
        }
        if (drbg->state == DRBG_UNINITIALISED) {
            RANDerr(RAND_F_RAND_DRBG_GENERATE, RAND_R_NOT_INSTANTIATED);
            return 0;
        }
    }

    if (outlen > drbg->max_request) {
        RANDerr(RAND_F_RAND_DRBG_GENERATE, RAND_R_REQUEST_TOO_LARGE_FOR_DRBG);
        return 0;
    }
    if (adinlen > drbg->max_adinlen) {
        RANDerr(RAND_F_RAND_DRBG_GENERATE, RAND_R_ADDITIONAL_INPUT_TOO_LONG);
        return 0;
    }

    if (drbg->fork_count != rand_fork_count) {
        drbg->fork_count = rand_fork_count;
        reseed_required = 1;
    }

    if (drbg->reseed_interval > 0) {
        if (drbg->reseed_gen_counter >= drbg->reseed_interval)
            reseed_required = 1;
    }
    if (drbg->reseed_time_interval > 0) {
        time_t now = time(NULL);
        if (now < drbg->reseed_time
            || now - drbg->reseed_time >= drbg->reseed_time_interval)
            reseed_required = 1;
    }
    if (drbg->parent != NULL) {
        unsigned int reseed_counter = tsan_load(&drbg->reseed_prop_counter);
        if (reseed_counter > 0
                && tsan_load(&drbg->parent->reseed_prop_counter)
                   != reseed_counter)
            reseed_required = 1;
    }

    if (reseed_required || prediction_resistance) {
        if (!RAND_DRBG_reseed(drbg, adin, adinlen, prediction_resistance)) {
            RANDerr(RAND_F_RAND_DRBG_GENERATE, RAND_R_RESEED_ERROR);
            return 0;
        }
        adin = NULL;
        adinlen = 0;
    }

    if (!drbg->meth->generate(drbg, out, outlen, adin, adinlen)) {
        drbg->state = DRBG_ERROR;
        RANDerr(RAND_F_RAND_DRBG_GENERATE, RAND_R_GENERATE_ERROR);
        return 0;
    }

    drbg->reseed_gen_counter++;

    return 1;
}