static int cleanup_bundling_task(ncInstance * instance, struct bundling_params_t *params, bundling_progress result)
{
    int rc = 0;
    char cmd[MAX_PATH] = { 0 };
    char buf[MAX_PATH] = { 0 };

    LOGINFO("[%s] bundling task result=%s\n", instance->instanceId, bundling_progress_names[result]);

    sem_p(inst_sem);
    {
        change_bundling_state(instance, result);
        copy_instances();
    }
    sem_v(inst_sem);

    if (params) {
        // if the result was failed or cancelled, clean up walrus state
        if ((result == BUNDLING_FAILED) || (result == BUNDLING_CANCELLED)) {
            if (!instance->bundleBucketExists) {
                snprintf(cmd, MAX_PATH, "%s -b %s -p %s --euca-auth", params->ncDeleteBundleCmd, params->bucketName, params->filePrefix);
            } else {
                snprintf(cmd, MAX_PATH, "%s -b %s -p %s --euca-auth --clear", params->ncDeleteBundleCmd, params->bucketName, params->filePrefix);
                instance->bundleBucketExists = 0;
            }

            // set up environment for euca2ools
            snprintf(buf, MAX_PATH, EUCALYPTUS_KEYS_DIR "/node-cert.pem", params->eucalyptusHomePath);
            setenv("EC2_CERT", buf, 1);

            snprintf(buf, MAX_PATH, "IGNORED");
            setenv("EC2_SECRET_KEY", buf, 1);

            snprintf(buf, MAX_PATH, EUCALYPTUS_KEYS_DIR "/cloud-cert.pem", params->eucalyptusHomePath);
            setenv("EUCALYPTUS_CERT", buf, 1);

            snprintf(buf, MAX_PATH, "%s", params->walrusURL);
            setenv("S3_URL", buf, 1);

            snprintf(buf, MAX_PATH, "%s", params->userPublicKey);
            setenv("EC2_ACCESS_KEY", buf, 1);

            snprintf(buf, MAX_PATH, "123456789012");
            setenv("EC2_USER_ID", buf, 1);

            snprintf(buf, MAX_PATH, EUCALYPTUS_KEYS_DIR "/node-cert.pem", params->eucalyptusHomePath);
            setenv("EUCA_CERT", buf, 1);

            snprintf(buf, MAX_PATH, EUCALYPTUS_KEYS_DIR "/node-pk.pem", params->eucalyptusHomePath);
            setenv("EUCA_PRIVATE_KEY", buf, 1);

            LOGDEBUG("running cmd '%s'\n", cmd);
            rc = system(cmd);
            rc = rc >> 8;
            if (rc) {
                LOGWARN("[%s] bucket cleanup cmd '%s' failed with rc '%d'\n", instance->instanceId, cmd, rc);
            }
        }

        EUCA_FREE(params->workPath);
        EUCA_FREE(params->bucketName);
        EUCA_FREE(params->filePrefix);
        EUCA_FREE(params->walrusURL);
        EUCA_FREE(params->userPublicKey);
        EUCA_FREE(params->diskPath);
        EUCA_FREE(params->eucalyptusHomePath);
        EUCA_FREE(params->ncBundleUploadCmd);
        EUCA_FREE(params->ncCheckBucketCmd);
        EUCA_FREE(params->ncDeleteBundleCmd);
        EUCA_FREE(params);
    }

    return ((result == BUNDLING_SUCCESS) ? EUCA_OK : EUCA_ERROR);
}