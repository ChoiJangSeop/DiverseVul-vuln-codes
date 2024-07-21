static int doBundleInstance(struct nc_state_t *nc, ncMetadata * pMeta, char *instanceId, char *bucketName, char *filePrefix, char *walrusURL,
                            char *userPublicKey, char *S3Policy, char *S3PolicySig)
{
    // sanity checking
    if (instanceId == NULL || bucketName == NULL || filePrefix == NULL || walrusURL == NULL || userPublicKey == NULL || S3Policy == NULL || S3PolicySig == NULL) {
        LOGERROR("[%s] bundling instance called with invalid parameters\n", ((instanceId == NULL) ? "UNKNOWN" : instanceId));
        return EUCA_ERROR;
    }
    // find the instance
    ncInstance *instance = find_instance(&global_instances, instanceId);
    if (instance == NULL) {
        LOGERROR("[%s] instance not found\n", instanceId);
        return EUCA_NOT_FOUND_ERROR;
    }
    // "marshall" thread parameters
    struct bundling_params_t *params = EUCA_ZALLOC(1, sizeof(struct bundling_params_t));
    if (params == NULL)
        return cleanup_bundling_task(instance, params, BUNDLING_FAILED);

    params->instance = instance;
    params->bucketName = strdup(bucketName);
    params->filePrefix = strdup(filePrefix);
    params->walrusURL = strdup(walrusURL);
    params->userPublicKey = strdup(userPublicKey);
    params->S3Policy = strdup(S3Policy);
    params->S3PolicySig = strdup(S3PolicySig);
    params->eucalyptusHomePath = strdup(nc->home);
    params->ncBundleUploadCmd = strdup(nc->ncBundleUploadCmd);
    params->ncCheckBucketCmd = strdup(nc->ncCheckBucketCmd);
    params->ncDeleteBundleCmd = strdup(nc->ncDeleteBundleCmd);

    params->workPath = strdup(instance->instancePath);

    // terminate the instance
    sem_p(inst_sem);
    instance->bundlingTime = time(NULL);
    change_state(instance, BUNDLING_SHUTDOWN);
    change_bundling_state(instance, BUNDLING_IN_PROGRESS);
    sem_v(inst_sem);

    int err = find_and_terminate_instance(instanceId);

    sem_p(inst_sem);
    copy_instances();
    sem_v(inst_sem);

    if (err != EUCA_OK) {
        EUCA_FREE(params);
        return err;
    }
    // do the rest in a thread
    pthread_attr_t tattr;
    pthread_t tid;
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &tattr, bundling_thread, (void *)params) != 0) {
        LOGERROR("[%s] failed to start VM bundling thread\n", instanceId);
        return cleanup_bundling_task(instance, params, BUNDLING_FAILED);
    }

    return EUCA_OK;
}