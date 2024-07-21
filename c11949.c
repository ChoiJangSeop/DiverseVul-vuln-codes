static void *bundling_thread(void *arg)
{
    struct bundling_params_t *params = (struct bundling_params_t *)arg;
    ncInstance *instance = params->instance;
    char cmd[MAX_PATH];
    char buf[MAX_PATH];

    LOGDEBUG("[%s] spawning bundling thread\n", instance->instanceId);
    LOGINFO("[%s] waiting for instance to shut down\n", instance->instanceId);
    // wait until monitor thread changes the state of the instance instance
    if (wait_state_transition(instance, BUNDLING_SHUTDOWN, BUNDLING_SHUTOFF)) {
        if (instance->bundleCanceled) { // cancel request came in while the instance was shutting down
            LOGINFO("[%s] cancelled while bundling instance\n", instance->instanceId);
            cleanup_bundling_task(instance, params, BUNDLING_CANCELLED);
        } else {
            LOGINFO("[%s] failed while bundling instance\n", instance->instanceId);
            cleanup_bundling_task(instance, params, BUNDLING_FAILED);
        }
        return NULL;
    }

    LOGINFO("[%s] started bundling instance\n", instance->instanceId);

    int rc = EUCA_OK;
    char bundlePath[MAX_PATH];
    bundlePath[0] = '\0';
    if (clone_bundling_backing(instance, params->filePrefix, bundlePath) != EUCA_OK) {
        LOGERROR("[%s] could not clone the instance image\n", instance->instanceId);
        cleanup_bundling_task(instance, params, BUNDLING_FAILED);
    } else {
        char prefixPath[MAX_PATH];
        snprintf(prefixPath, MAX_PATH, "%s/%s", instance->instancePath, params->filePrefix);
        if (strcmp(bundlePath, prefixPath) != 0 && rename(bundlePath, prefixPath) != 0) {
            LOGERROR("[%s] could not rename from %s to %s\n", instance->instanceId, bundlePath, prefixPath);
            cleanup_bundling_task(instance, params, BUNDLING_FAILED);
            return NULL;
        }
        // USAGE: euca-nc-bundle-upload -i <image_path> -d <working dir> -b <bucket>
        int pid, status;

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

        // check to see if the bucket exists in advance
        snprintf(cmd, MAX_PATH, "%s -b %s --euca-auth", params->ncCheckBucketCmd, params->bucketName);
        LOGDEBUG("[%s] running cmd '%s'\n", instance->instanceId, cmd);
        rc = system(cmd);
        rc = rc >> 8;
        instance->bundleBucketExists = rc;

        if (instance->bundleCanceled) {
            LOGINFO("[%s] bundle task canceled; terminating bundling thread\n", instance->instanceId);
            cleanup_bundling_task(instance, params, BUNDLING_CANCELLED);
            return NULL;
        }

        pid = fork();
        if (!pid) {
            LOGDEBUG("[%s] running cmd '%s -i %s -d %s -b %s -c %s --policysignature %s --euca-auth'\n", instance->instanceId,
                     params->ncBundleUploadCmd, prefixPath, params->workPath, params->bucketName, params->S3Policy, params->S3PolicySig);
            exit(execlp
                 (params->ncBundleUploadCmd, params->ncBundleUploadCmd, "-i", prefixPath, "-d", params->workPath, "-b", params->bucketName, "-c",
                  params->S3Policy, "--policysignature", params->S3PolicySig, "--euca-auth", NULL));
        } else {
            instance->bundlePid = pid;
            rc = waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                rc = WEXITSTATUS(status);
            } else {
                rc = -1;
            }
        }

        if (rc == 0) {
            cleanup_bundling_task(instance, params, BUNDLING_SUCCESS);
            LOGINFO("[%s] finished bundling instance\n", instance->instanceId);
        } else if (rc == -1) {
            // bundler child was cancelled (killed), but should report it as failed
            cleanup_bundling_task(instance, params, BUNDLING_FAILED);
            LOGINFO("[%s] cancelled while bundling instance (rc=%d)\n", instance->instanceId, rc);
        } else {
            cleanup_bundling_task(instance, params, BUNDLING_FAILED);
            LOGINFO("[%s] failed while bundling instance (rc=%d)\n", instance->instanceId, rc);
        }
    }
    return NULL;
}