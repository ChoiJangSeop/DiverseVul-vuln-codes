int ha_myisam::repair(THD *thd, MI_CHECK &param, bool do_optimize)
{
  int error=0;
  uint local_testflag=param.testflag;
  bool optimize_done= !do_optimize, statistics_done=0;
  const char *old_proc_info=thd->proc_info;
  char fixed_name[FN_REFLEN];
  MYISAM_SHARE* share = file->s;
  ha_rows rows= file->state->records;
  DBUG_ENTER("ha_myisam::repair");

  param.db_name=    table->s->db.str;
  param.table_name= table->alias;
  param.using_global_keycache = 1;
  param.thd= thd;
  param.tmpdir= &mysql_tmpdir_list;
  param.out_flag= 0;
  strmov(fixed_name,file->filename);

  // Release latches since this can take a long time
  ha_release_temporary_latches(thd);

  // Don't lock tables if we have used LOCK TABLE
  if (! thd->locked_tables_mode &&
      mi_lock_database(file, table->s->tmp_table ? F_EXTRA_LCK : F_WRLCK))
  {
    mi_check_print_error(&param,ER(ER_CANT_LOCK),my_errno);
    DBUG_RETURN(HA_ADMIN_FAILED);
  }

  if (!do_optimize ||
      ((file->state->del || share->state.split != file->state->records) &&
       (!(param.testflag & T_QUICK) ||
	!(share->state.changed & STATE_NOT_OPTIMIZED_KEYS))))
  {
    ulonglong key_map= ((local_testflag & T_CREATE_MISSING_KEYS) ?
			mi_get_mask_all_keys_active(share->base.keys) :
			share->state.key_map);
    uint testflag=param.testflag;
#ifdef HAVE_MMAP
    bool remap= test(share->file_map);
    /*
      mi_repair*() functions family use file I/O even if memory
      mapping is available.

      Since mixing mmap I/O and file I/O may cause various artifacts,
      memory mapping must be disabled.
    */
    if (remap)
      mi_munmap_file(file);
#endif
    if (mi_test_if_sort_rep(file,file->state->records,key_map,0) &&
	(local_testflag & T_REP_BY_SORT))
    {
      local_testflag|= T_STATISTICS;
      param.testflag|= T_STATISTICS;		// We get this for free
      statistics_done=1;
      if (THDVAR(thd, repair_threads)>1)
      {
        char buf[40];
        /* TODO: respect myisam_repair_threads variable */
        my_snprintf(buf, 40, "Repair with %d threads", my_count_bits(key_map));
        thd_proc_info(thd, buf);
        error = mi_repair_parallel(&param, file, fixed_name,
            param.testflag & T_QUICK);
        thd_proc_info(thd, "Repair done"); // to reset proc_info, as
                                      // it was pointing to local buffer
      }
      else
      {
        thd_proc_info(thd, "Repair by sorting");
        error = mi_repair_by_sort(&param, file, fixed_name,
            param.testflag & T_QUICK);
      }
    }
    else
    {
      thd_proc_info(thd, "Repair with keycache");
      param.testflag &= ~T_REP_BY_SORT;
      error=  mi_repair(&param, file, fixed_name,
			param.testflag & T_QUICK);
    }
#ifdef HAVE_MMAP
    if (remap)
      mi_dynmap_file(file, file->state->data_file_length);
#endif
    param.testflag=testflag;
    optimize_done=1;
  }
  if (!error)
  {
    if ((local_testflag & T_SORT_INDEX) &&
	(share->state.changed & STATE_NOT_SORTED_PAGES))
    {
      optimize_done=1;
      thd_proc_info(thd, "Sorting index");
      error=mi_sort_index(&param,file,fixed_name);
    }
    if (!statistics_done && (local_testflag & T_STATISTICS))
    {
      if (share->state.changed & STATE_NOT_ANALYZED)
      {
	optimize_done=1;
	thd_proc_info(thd, "Analyzing");
	error = chk_key(&param, file);
      }
      else
	local_testflag&= ~T_STATISTICS;		// Don't update statistics
    }
  }
  thd_proc_info(thd, "Saving state");
  if (!error)
  {
    if ((share->state.changed & STATE_CHANGED) || mi_is_crashed(file))
    {
      share->state.changed&= ~(STATE_CHANGED | STATE_CRASHED |
			       STATE_CRASHED_ON_REPAIR);
      file->update|=HA_STATE_CHANGED | HA_STATE_ROW_CHANGED;
    }
    /*
      the following 'if', thought conceptually wrong,
      is a useful optimization nevertheless.
    */
    if (file->state != &file->s->state.state)
      file->s->state.state = *file->state;
    if (file->s->base.auto_key)
      update_auto_increment_key(&param, file, 1);
    if (optimize_done)
      error = update_state_info(&param, file,
				UPDATE_TIME | UPDATE_OPEN_COUNT |
				(local_testflag &
				 T_STATISTICS ? UPDATE_STAT : 0));
    info(HA_STATUS_NO_LOCK | HA_STATUS_TIME | HA_STATUS_VARIABLE |
	 HA_STATUS_CONST);
    if (rows != file->state->records && ! (param.testflag & T_VERY_SILENT))
    {
      char llbuff[22],llbuff2[22];
      mi_check_print_warning(&param,"Number of rows changed from %s to %s",
			     llstr(rows,llbuff),
			     llstr(file->state->records,llbuff2));
    }
  }
  else
  {
    mi_mark_crashed_on_repair(file);
    file->update |= HA_STATE_CHANGED | HA_STATE_ROW_CHANGED;
    update_state_info(&param, file, 0);
  }
  thd_proc_info(thd, old_proc_info);
  if (! thd->locked_tables_mode)
    mi_lock_database(file,F_UNLCK);
  DBUG_RETURN(error ? HA_ADMIN_FAILED :
	      !optimize_done ? HA_ADMIN_ALREADY_DONE : HA_ADMIN_OK);
}