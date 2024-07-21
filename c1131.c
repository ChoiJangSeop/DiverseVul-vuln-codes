sds genRedisInfoString(void) {
    sds info;
    time_t uptime = time(NULL)-server.stat_starttime;
    int j;
    char hmem[64];
    struct rusage self_ru, c_ru;

    getrusage(RUSAGE_SELF, &self_ru);
    getrusage(RUSAGE_CHILDREN, &c_ru);

    bytesToHuman(hmem,zmalloc_used_memory());
    info = sdscatprintf(sdsempty(),
        "redis_version:%s\r\n"
        "redis_git_sha1:%s\r\n"
        "redis_git_dirty:%d\r\n"
        "arch_bits:%s\r\n"
        "multiplexing_api:%s\r\n"
        "process_id:%ld\r\n"
        "uptime_in_seconds:%ld\r\n"
        "uptime_in_days:%ld\r\n"
        "lru_clock:%ld\r\n"
        "used_cpu_sys:%.2f\r\n"
        "used_cpu_user:%.2f\r\n"
        "used_cpu_sys_childrens:%.2f\r\n"
        "used_cpu_user_childrens:%.2f\r\n"
        "connected_clients:%d\r\n"
        "connected_slaves:%d\r\n"
        "blocked_clients:%d\r\n"
        "used_memory:%zu\r\n"
        "used_memory_human:%s\r\n"
        "used_memory_rss:%zu\r\n"
        "mem_fragmentation_ratio:%.2f\r\n"
        "use_tcmalloc:%d\r\n"
        "loading:%d\r\n"
        "aof_enabled:%d\r\n"
        "changes_since_last_save:%lld\r\n"
        "bgsave_in_progress:%d\r\n"
        "last_save_time:%ld\r\n"
        "bgrewriteaof_in_progress:%d\r\n"
        "total_connections_received:%lld\r\n"
        "total_commands_processed:%lld\r\n"
        "expired_keys:%lld\r\n"
        "evicted_keys:%lld\r\n"
        "keyspace_hits:%lld\r\n"
        "keyspace_misses:%lld\r\n"
        "hash_max_zipmap_entries:%zu\r\n"
        "hash_max_zipmap_value:%zu\r\n"
        "pubsub_channels:%ld\r\n"
        "pubsub_patterns:%u\r\n"
        "vm_enabled:%d\r\n"
        "role:%s\r\n"
        ,REDIS_VERSION,
        redisGitSHA1(),
        strtol(redisGitDirty(),NULL,10) > 0,
        (sizeof(long) == 8) ? "64" : "32",
        aeGetApiName(),
        (long) getpid(),
        uptime,
        uptime/(3600*24),
        (unsigned long) server.lruclock,
        (float)self_ru.ru_utime.tv_sec+(float)self_ru.ru_utime.tv_usec/1000000,
        (float)self_ru.ru_stime.tv_sec+(float)self_ru.ru_stime.tv_usec/1000000,
        (float)c_ru.ru_utime.tv_sec+(float)c_ru.ru_utime.tv_usec/1000000,
        (float)c_ru.ru_stime.tv_sec+(float)c_ru.ru_stime.tv_usec/1000000,
        listLength(server.clients)-listLength(server.slaves),
        listLength(server.slaves),
        server.bpop_blocked_clients,
        zmalloc_used_memory(),
        hmem,
        zmalloc_get_rss(),
        zmalloc_get_fragmentation_ratio(),
#ifdef USE_TCMALLOC
        1,
#else
        0,
#endif
        server.loading,
        server.appendonly,
        server.dirty,
        server.bgsavechildpid != -1,
        server.lastsave,
        server.bgrewritechildpid != -1,
        server.stat_numconnections,
        server.stat_numcommands,
        server.stat_expiredkeys,
        server.stat_evictedkeys,
        server.stat_keyspace_hits,
        server.stat_keyspace_misses,
        server.hash_max_zipmap_entries,
        server.hash_max_zipmap_value,
        dictSize(server.pubsub_channels),
        listLength(server.pubsub_patterns),
        server.vm_enabled != 0,
        server.masterhost == NULL ? "master" : "slave"
    );
    if (server.masterhost) {
        info = sdscatprintf(info,
            "master_host:%s\r\n"
            "master_port:%d\r\n"
            "master_link_status:%s\r\n"
            "master_last_io_seconds_ago:%d\r\n"
            "master_sync_in_progress:%d\r\n"
            ,server.masterhost,
            server.masterport,
            (server.replstate == REDIS_REPL_CONNECTED) ?
                "up" : "down",
            server.master ? ((int)(time(NULL)-server.master->lastinteraction)) : -1,
            server.replstate == REDIS_REPL_TRANSFER
        );

        if (server.replstate == REDIS_REPL_TRANSFER) {
            info = sdscatprintf(info,
                "master_sync_left_bytes:%ld\r\n"
                "master_sync_last_io_seconds_ago:%d\r\n"
                ,(long)server.repl_transfer_left,
                (int)(time(NULL)-server.repl_transfer_lastio)
            );
        }
    }
    if (server.vm_enabled) {
        lockThreadedIO();
        info = sdscatprintf(info,
            "vm_conf_max_memory:%llu\r\n"
            "vm_conf_page_size:%llu\r\n"
            "vm_conf_pages:%llu\r\n"
            "vm_stats_used_pages:%llu\r\n"
            "vm_stats_swapped_objects:%llu\r\n"
            "vm_stats_swappin_count:%llu\r\n"
            "vm_stats_swappout_count:%llu\r\n"
            "vm_stats_io_newjobs_len:%lu\r\n"
            "vm_stats_io_processing_len:%lu\r\n"
            "vm_stats_io_processed_len:%lu\r\n"
            "vm_stats_io_active_threads:%lu\r\n"
            "vm_stats_blocked_clients:%lu\r\n"
            ,(unsigned long long) server.vm_max_memory,
            (unsigned long long) server.vm_page_size,
            (unsigned long long) server.vm_pages,
            (unsigned long long) server.vm_stats_used_pages,
            (unsigned long long) server.vm_stats_swapped_objects,
            (unsigned long long) server.vm_stats_swapins,
            (unsigned long long) server.vm_stats_swapouts,
            (unsigned long) listLength(server.io_newjobs),
            (unsigned long) listLength(server.io_processing),
            (unsigned long) listLength(server.io_processed),
            (unsigned long) server.io_active_threads,
            (unsigned long) server.vm_blocked_clients
        );
        unlockThreadedIO();
    }
    if (server.loading) {
        double perc;
        time_t eta, elapsed;
        off_t remaining_bytes = server.loading_total_bytes-
                                server.loading_loaded_bytes;

        perc = ((double)server.loading_loaded_bytes /
               server.loading_total_bytes) * 100;

        elapsed = time(NULL)-server.loading_start_time;
        if (elapsed == 0) {
            eta = 1; /* A fake 1 second figure if we don't have enough info */
        } else {
            eta = (elapsed*remaining_bytes)/server.loading_loaded_bytes;
        }

        info = sdscatprintf(info,
            "loading_start_time:%ld\r\n"
            "loading_total_bytes:%llu\r\n"
            "loading_loaded_bytes:%llu\r\n"
            "loading_loaded_perc:%.2f\r\n"
            "loading_eta_seconds:%ld\r\n"
            ,(unsigned long) server.loading_start_time,
            (unsigned long long) server.loading_total_bytes,
            (unsigned long long) server.loading_loaded_bytes,
            perc,
            eta
        );
    }
    for (j = 0; j < server.dbnum; j++) {
        long long keys, vkeys;

        keys = dictSize(server.db[j].dict);
        vkeys = dictSize(server.db[j].expires);
        if (keys || vkeys) {
            info = sdscatprintf(info, "db%d:keys=%lld,expires=%lld\r\n",
                j, keys, vkeys);
        }
    }
    return info;
}