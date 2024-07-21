int prepareForShutdown() {
    redisLog(REDIS_WARNING,"User requested shutdown, saving DB...");
    /* Kill the saving child if there is a background saving in progress.
       We want to avoid race conditions, for instance our saving child may
       overwrite the synchronous saving did by SHUTDOWN. */
    if (server.bgsavechildpid != -1) {
        redisLog(REDIS_WARNING,"There is a live saving child. Killing it!");
        kill(server.bgsavechildpid,SIGKILL);
        rdbRemoveTempFile(server.bgsavechildpid);
    }
    if (server.appendonly) {
        /* Append only file: fsync() the AOF and exit */
        aof_fsync(server.appendfd);
        if (server.vm_enabled) unlink(server.vm_swap_file);
    } else if (server.saveparamslen > 0) {
        /* Snapshotting. Perform a SYNC SAVE and exit */
        if (rdbSave(server.dbfilename) != REDIS_OK) {
            /* Ooops.. error saving! The best we can do is to continue
             * operating. Note that if there was a background saving process,
             * in the next cron() Redis will be notified that the background
             * saving aborted, handling special stuff like slaves pending for
             * synchronization... */
            redisLog(REDIS_WARNING,"Error trying to save the DB, can't exit");
            return REDIS_ERR;
        }
    } else {
        redisLog(REDIS_WARNING,"Not saving DB.");
    }
    if (server.daemonize) unlink(server.pidfile);
    redisLog(REDIS_WARNING,"Server exit now, bye bye...");
    return REDIS_OK;
}