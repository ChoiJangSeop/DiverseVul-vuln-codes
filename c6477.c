static apr_status_t run_watchdog(int state, void *baton, apr_pool_t *ptemp)
{
    md_watchdog *wd = baton;
    apr_status_t rv = APR_SUCCESS;
    md_job_t *job;
    apr_time_t next_run, now;
    int restart = 0;
    int i;
    
    switch (state) {
        case AP_WATCHDOG_STATE_STARTING:
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, wd->s, APLOGNO(10054)
                         "md watchdog start, auto drive %d mds", wd->jobs->nelts);
            assert(wd->reg);
        
            for (i = 0; i < wd->jobs->nelts; ++i) {
                job = APR_ARRAY_IDX(wd->jobs, i, md_job_t *);
                load_job_props(wd->reg, job, ptemp);
            }
            break;
        case AP_WATCHDOG_STATE_RUNNING:
        
            wd->next_change = 0;
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, wd->s, APLOGNO(10055)
                         "md watchdog run, auto drive %d mds", wd->jobs->nelts);
                         
            /* normally, we'd like to run at least twice a day */
            next_run = apr_time_now() + apr_time_from_sec(MD_SECS_PER_DAY / 2);

            /* Check on all the jobs we have */
            for (i = 0; i < wd->jobs->nelts; ++i) {
                job = APR_ARRAY_IDX(wd->jobs, i, md_job_t *);
                
                rv = check_job(wd, job, ptemp);

                if (job->need_restart && !job->restart_processed) {
                    restart = 1;
                }
                if (job->next_check && job->next_check < next_run) {
                    next_run = job->next_check;
                }
            }

            now = apr_time_now();
            if (APLOGdebug(wd->s)) {
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, wd->s, APLOGNO(10107)
                             "next run in %s", md_print_duration(ptemp, next_run - now));
            }
            wd_set_interval(wd->watchdog, next_run - now, wd, run_watchdog);
            break;
            
        case AP_WATCHDOG_STATE_STOPPING:
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, wd->s, APLOGNO(10058)
                         "md watchdog stopping");
            break;
    }

    if (restart) {
        const char *action, *names = "";
        int n;
        
        for (i = 0, n = 0; i < wd->jobs->nelts; ++i) {
            job = APR_ARRAY_IDX(wd->jobs, i, md_job_t *);
            if (job->need_restart && !job->restart_processed) {
                names = apr_psprintf(ptemp, "%s%s%s", names, n? " " : "", job->md->name);
                ++n;
            }
        }

        if (n > 0) {
            int notified = 1;

            /* Run notify command for ready MDs (if configured) and persist that
             * we have done so. This process might be reaped after n requests or die
             * of another cause. The one taking over the watchdog need to notify again.
             */
            if (wd->mc->notify_cmd) {
                const char * const *argv;
                const char *cmdline;
                int exit_code;
                
                cmdline = apr_psprintf(ptemp, "%s %s", wd->mc->notify_cmd, names); 
                apr_tokenize_to_argv(cmdline, (char***)&argv, ptemp);
                if (APR_SUCCESS == (rv = md_util_exec(ptemp, argv[0], argv, &exit_code))) {
                    ap_log_error(APLOG_MARK, APLOG_DEBUG, rv, wd->s, APLOGNO(10108) 
                                 "notify command '%s' returned %d", 
                                 wd->mc->notify_cmd, exit_code);
                }
                else {
                    ap_log_error(APLOG_MARK, APLOG_ERR, rv, wd->s, APLOGNO(10109) 
                                 "executing configured MDNotifyCmd %s", wd->mc->notify_cmd);
                    notified = 0;
                } 
            }
            
            if (notified) {
                /* persist the jobs that were notified */
                for (i = 0, n = 0; i < wd->jobs->nelts; ++i) {
                    job = APR_ARRAY_IDX(wd->jobs, i, md_job_t *);
                    if (job->need_restart && !job->restart_processed) {
                        job->restart_processed = 1;
                        save_job_props(wd->reg, job, ptemp);
                    }
                }
            }
            
            /* FIXME: the server needs to start gracefully to take the new certificate in.
             * This poses a variety of problems to solve satisfactory for everyone:
             * - I myself, have no implementation for Windows 
             * - on *NIX, child processes run with less privileges, preventing
             *   the signal based restart trigger to work
             * - admins want better control of timing windows for restarts, e.g.
             *   during less busy hours/days.
             */
            rv = md_server_graceful(ptemp, wd->s);
            if (APR_ENOTIMPL == rv) {
                /* self-graceful restart not supported in this setup */
                action = " and changes will be activated on next (graceful) server restart.";
            }
            else {
                action = " and server has been asked to restart now.";
            }
            ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, wd->s, APLOGNO(10059) 
                         "The Managed Domain%s %s %s been setup%s",
                         (n > 1)? "s" : "", names, (n > 1)? "have" : "has", action);
        }
    }
    
    return APR_SUCCESS;
}