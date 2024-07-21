static int myisamchk(MI_CHECK *param, char * filename)
{
  int error,lock_type,recreate;
  int rep_quick= param->testflag & (T_QUICK | T_FORCE_UNIQUENESS);
  MI_INFO *info;
  File datafile;
  char llbuff[22],llbuff2[22];
  my_bool state_updated=0;
  MYISAM_SHARE *share;
  DBUG_ENTER("myisamchk");

  param->out_flag=error=param->warning_printed=param->error_printed=
    recreate=0;
  datafile=0;
  param->isam_file_name=filename;		/* For error messages */
  if (!(info=mi_open(filename,
		     (param->testflag & (T_DESCRIPT | T_READONLY)) ?
		     O_RDONLY : O_RDWR,
		     HA_OPEN_FOR_REPAIR |
		     ((param->testflag & T_WAIT_FOREVER) ?
		      HA_OPEN_WAIT_IF_LOCKED :
		      (param->testflag & T_DESCRIPT) ?
		      HA_OPEN_IGNORE_IF_LOCKED : HA_OPEN_ABORT_IF_LOCKED))))
  {
    /* Avoid twice printing of isam file name */
    param->error_printed=1;
    switch (my_errno) {
    case HA_ERR_CRASHED:
      mi_check_print_error(param,"'%s' doesn't have a correct index definition. You need to recreate it before you can do a repair",filename);
      break;
    case HA_ERR_NOT_A_TABLE:
      mi_check_print_error(param,"'%s' is not a MyISAM-table",filename);
      break;
    case HA_ERR_CRASHED_ON_USAGE:
      mi_check_print_error(param,"'%s' is marked as crashed",filename);
      break;
    case HA_ERR_CRASHED_ON_REPAIR:
      mi_check_print_error(param,"'%s' is marked as crashed after last repair",filename);
      break;
    case HA_ERR_OLD_FILE:
      mi_check_print_error(param,"'%s' is an old type of MyISAM-table", filename);
      break;
    case HA_ERR_END_OF_FILE:
      mi_check_print_error(param,"Couldn't read complete header from '%s'", filename);
      break;
    case EAGAIN:
      mi_check_print_error(param,"'%s' is locked. Use -w to wait until unlocked",filename);
      break;
    case ENOENT:
      mi_check_print_error(param,"File '%s' doesn't exist",filename);
      break;
    case EACCES:
      mi_check_print_error(param,"You don't have permission to use '%s'",filename);
      break;
    default:
      mi_check_print_error(param,"%d when opening MyISAM-table '%s'",
		  my_errno,filename);
      break;
    }
    DBUG_RETURN(1);
  }
  share=info->s;
  share->options&= ~HA_OPTION_READ_ONLY_DATA; /* We are modifing it */
  share->tot_locks-= share->r_locks;
  share->r_locks=0;

  /*
    Skip the checking of the file if:
    We are using --fast and the table is closed properly
    We are using --check-only-changed-tables and the table hasn't changed
  */
  if (param->testflag & (T_FAST | T_CHECK_ONLY_CHANGED))
  {
    my_bool need_to_check= mi_is_crashed(info) || share->state.open_count != 0;

    if ((param->testflag & (T_REP_ANY | T_SORT_RECORDS)) &&
	((share->state.changed & (STATE_CHANGED | STATE_CRASHED |
				  STATE_CRASHED_ON_REPAIR) ||
	  !(param->testflag & T_CHECK_ONLY_CHANGED))))
      need_to_check=1;

    if (info->s->base.keys && info->state->records)
    {
      if ((param->testflag & T_STATISTICS) &&
          (share->state.changed & STATE_NOT_ANALYZED))
        need_to_check=1;
      if ((param->testflag & T_SORT_INDEX) &&
          (share->state.changed & STATE_NOT_SORTED_PAGES))
        need_to_check=1;
      if ((param->testflag & T_REP_BY_SORT) &&
          (share->state.changed & STATE_NOT_OPTIMIZED_KEYS))
        need_to_check=1;
    }
    if ((param->testflag & T_CHECK_ONLY_CHANGED) &&
	(share->state.changed & (STATE_CHANGED | STATE_CRASHED |
				 STATE_CRASHED_ON_REPAIR)))
      need_to_check=1;
    if (!need_to_check)
    {
      if (!(param->testflag & T_SILENT) || param->testflag & T_INFO)
	printf("MyISAM file: %s is already checked\n",filename);
      if (mi_close(info))
      {
	mi_check_print_error(param,"%d when closing MyISAM-table '%s'",
			     my_errno,filename);
	DBUG_RETURN(1);
      }
      DBUG_RETURN(0);
    }
  }
  if ((param->testflag & (T_REP_ANY | T_STATISTICS |
			  T_SORT_RECORDS | T_SORT_INDEX)) &&
      (((param->testflag & T_UNPACK) &&
	share->data_file_type == COMPRESSED_RECORD) ||
       mi_uint2korr(share->state.header.state_info_length) !=
       MI_STATE_INFO_SIZE ||
       mi_uint2korr(share->state.header.base_info_length) !=
       MI_BASE_INFO_SIZE ||
       mi_is_any_intersect_keys_active(param->keys_in_use, share->base.keys,
                                       ~share->state.key_map) ||
       test_if_almost_full(info) ||
       info->s->state.header.file_version[3] != myisam_file_magic[3] ||
       (set_collation &&
        set_collation->number != share->state.header.language) ||
       myisam_block_size != MI_KEY_BLOCK_LENGTH))
  {
    if (set_collation)
      param->language= set_collation->number;
    if (recreate_table(param, &info,filename))
    {
      (void) fprintf(stderr,
		   "MyISAM-table '%s' is not fixed because of errors\n",
	      filename);
      return(-1);
    }
    recreate=1;
    if (!(param->testflag & T_REP_ANY))
    {
      param->testflag|=T_REP_BY_SORT;		/* if only STATISTICS */
      if (!(param->testflag & T_SILENT))
	printf("- '%s' has old table-format. Recreating index\n",filename);
      rep_quick|=T_QUICK;
    }
    share=info->s;
    share->tot_locks-= share->r_locks;
    share->r_locks=0;
  }

  if (param->testflag & T_DESCRIPT)
  {
    param->total_files++;
    param->total_records+=info->state->records;
    param->total_deleted+=info->state->del;
    descript(param, info, filename);
  }
  else
  {
    if (!stopwords_inited++)
      ft_init_stopwords();

    if (!(param->testflag & T_READONLY))
      lock_type = F_WRLCK;			/* table is changed */
    else
      lock_type= F_RDLCK;
    if (info->lock_type == F_RDLCK)
      info->lock_type=F_UNLCK;			/* Read only table */
    if (_mi_readinfo(info,lock_type,0))
    {
      mi_check_print_error(param,"Can't lock indexfile of '%s', error: %d",
		  filename,my_errno);
      param->error_printed=0;
      goto end2;
    }
    /*
      _mi_readinfo() has locked the table.
      We mark the table as locked (without doing file locks) to be able to
      use functions that only works on locked tables (like row caching).
    */
    mi_lock_database(info, F_EXTRA_LCK);
    datafile=info->dfile;

    if (param->testflag & (T_REP_ANY | T_SORT_RECORDS | T_SORT_INDEX))
    {
      if (param->testflag & T_REP_ANY)
      {
	ulonglong tmp=share->state.key_map;
	mi_copy_keys_active(share->state.key_map, share->base.keys,
                            param->keys_in_use);
	if (tmp != share->state.key_map)
	  info->update|=HA_STATE_CHANGED;
      }
      if (rep_quick && chk_del(param, info, param->testflag & ~T_VERBOSE))
      {
	if (param->testflag & T_FORCE_CREATE)
	{
	  rep_quick=0;
	  mi_check_print_info(param,"Creating new data file\n");
	}
	else
	{
	  error=1;
	  mi_check_print_error(param,
			       "Quick-recover aborted; Run recovery without switch 'q'");
	}
      }
      if (!error)
      {
	if ((param->testflag & (T_REP_BY_SORT | T_REP_PARALLEL)) &&
	    (mi_is_any_key_active(share->state.key_map) ||
	     (rep_quick && !param->keys_in_use && !recreate)) &&
	    mi_test_if_sort_rep(info, info->state->records,
				info->s->state.key_map,
				param->force_sort))
	{
          if (param->testflag & T_REP_BY_SORT)
            error=mi_repair_by_sort(param,info,filename,rep_quick);
          else
            error=mi_repair_parallel(param,info,filename,rep_quick);
	  state_updated=1;
	}
	else if (param->testflag & T_REP_ANY)
	  error=mi_repair(param, info,filename,rep_quick);
      }
      if (!error && param->testflag & T_SORT_RECORDS)
      {
	/*
	  The data file is nowadays reopened in the repair code so we should
	  soon remove the following reopen-code
	*/
#ifndef TO_BE_REMOVED
	if (param->out_flag & O_NEW_DATA)
	{			/* Change temp file to org file */
	  (void) my_close(info->dfile,MYF(MY_WME)); /* Close new file */
	  error|=change_to_newfile(filename, MI_NAME_DEXT, DATA_TMP_EXT, MYF(0));
	  if (mi_open_datafile(info,info->s, NULL, -1))
	    error=1;
	  param->out_flag&= ~O_NEW_DATA; /* We are using new datafile */
	  param->read_cache.file=info->dfile;
	}
#endif
	if (! error)
	{
	  uint key;
	  /*
	    We can't update the index in mi_sort_records if we have a
	    prefix compressed or fulltext index
	  */
	  my_bool update_index=1;
	  for (key=0 ; key < share->base.keys; key++)
	    if (share->keyinfo[key].flag & (HA_BINARY_PACK_KEY|HA_FULLTEXT))
	      update_index=0;

	  error=mi_sort_records(param,info,filename,param->opt_sort_key,
                             /* what is the following parameter for ? */
				(my_bool) !(param->testflag & T_REP),
				update_index);
	  datafile=info->dfile;	/* This is now locked */
	  if (!error && !update_index)
	  {
	    if (param->verbose)
	      puts("Table had a compressed index;  We must now recreate the index");
	    error=mi_repair_by_sort(param,info,filename,1);
	  }
	}
      }
      if (!error && param->testflag & T_SORT_INDEX)
	error=mi_sort_index(param,info,filename);
      if (!error)
	share->state.changed&= ~(STATE_CHANGED | STATE_CRASHED |
				 STATE_CRASHED_ON_REPAIR);
      else
	mi_mark_crashed(info);
    }
    else if ((param->testflag & T_CHECK) || !(param->testflag & T_AUTO_INC))
    {
      if (!(param->testflag & T_SILENT) || param->testflag & T_INFO)
	printf("Checking MyISAM file: %s\n",filename);
      if (!(param->testflag & T_SILENT))
	printf("Data records: %7s   Deleted blocks: %7s\n",
	       llstr(info->state->records,llbuff),
	       llstr(info->state->del,llbuff2));
      error =chk_status(param,info);
      mi_intersect_keys_active(share->state.key_map, param->keys_in_use);
      error =chk_size(param,info);
      if (!error || !(param->testflag & (T_FAST | T_FORCE_CREATE)))
	error|=chk_del(param, info,param->testflag);
      if ((!error || (!(param->testflag & (T_FAST | T_FORCE_CREATE)) &&
		      !param->start_check_pos)))
      {
	error|=chk_key(param, info);
	if (!error && (param->testflag & (T_STATISTICS | T_AUTO_INC)))
	  error=update_state_info(param, info,
				  ((param->testflag & T_STATISTICS) ?
				   UPDATE_STAT : 0) |
				  ((param->testflag & T_AUTO_INC) ?
				   UPDATE_AUTO_INC : 0));
      }
      if ((!rep_quick && !error) ||
	  !(param->testflag & (T_FAST | T_FORCE_CREATE)))
      {
	if (param->testflag & (T_EXTEND | T_MEDIUM))
	  (void) init_key_cache(dflt_key_cache,opt_key_cache_block_size,
                              param->use_buffers, 0, 0);
	(void) init_io_cache(&param->read_cache,datafile,
			   (uint) param->read_buffer_length,
			   READ_CACHE,
			   (param->start_check_pos ?
			    param->start_check_pos :
			    share->pack.header_length),
			   1,
			   MYF(MY_WME));
	lock_memory(param);
	if ((info->s->options & (HA_OPTION_PACK_RECORD |
				 HA_OPTION_COMPRESS_RECORD)) ||
	    (param->testflag & (T_EXTEND | T_MEDIUM)))
	  error|=chk_data_link(param, info, param->testflag & T_EXTEND);
	error|=flush_blocks(param, share->key_cache, share->kfile);
	(void) end_io_cache(&param->read_cache);
      }
      if (!error)
      {
	if ((share->state.changed & STATE_CHANGED) &&
	    (param->testflag & T_UPDATE_STATE))
	  info->update|=HA_STATE_CHANGED | HA_STATE_ROW_CHANGED;
	share->state.changed&= ~(STATE_CHANGED | STATE_CRASHED |
				 STATE_CRASHED_ON_REPAIR);
      }
      else if (!mi_is_crashed(info) &&
	       (param->testflag & T_UPDATE_STATE))
      {						/* Mark crashed */
	mi_mark_crashed(info);
	info->update|=HA_STATE_CHANGED | HA_STATE_ROW_CHANGED;
      }
    }
  }
  if ((param->testflag & T_AUTO_INC) ||
      ((param->testflag & T_REP_ANY) && info->s->base.auto_key))
    update_auto_increment_key(param, info,
			      (my_bool) !test(param->testflag & T_AUTO_INC));

  if (!(param->testflag & T_DESCRIPT))
  {
    if (info->update & HA_STATE_CHANGED && ! (param->testflag & T_READONLY))
      error|=update_state_info(param, info,
			       UPDATE_OPEN_COUNT |
			       (((param->testflag & T_REP_ANY) ?
				 UPDATE_TIME : 0) |
				(state_updated ? UPDATE_STAT : 0) |
				((param->testflag & T_SORT_RECORDS) ?
				 UPDATE_SORT : 0)));
    (void) lock_file(param, share->kfile,0L,F_UNLCK,"indexfile",filename);
    info->update&= ~HA_STATE_CHANGED;
  }
  mi_lock_database(info, F_UNLCK);
end2:
  if (mi_close(info))
  {
    mi_check_print_error(param,"%d when closing MyISAM-table '%s'",my_errno,filename);
    DBUG_RETURN(1);
  }
  if (error == 0)
  {
    if (param->out_flag & O_NEW_DATA)
      error|=change_to_newfile(filename,MI_NAME_DEXT,DATA_TMP_EXT,
			       ((param->testflag & T_BACKUP_DATA) ?
				MYF(MY_REDEL_MAKE_BACKUP) : MYF(0)));
    if (param->out_flag & O_NEW_INDEX)
      error|=change_to_newfile(filename, MI_NAME_IEXT, INDEX_TMP_EXT, MYF(0));
  }
  (void) fflush(stdout); (void) fflush(stderr);
  if (param->error_printed)
  {
    if (param->testflag & (T_REP_ANY | T_SORT_RECORDS | T_SORT_INDEX))
    {
      (void) fprintf(stderr,
		   "MyISAM-table '%s' is not fixed because of errors\n",
		   filename);
      if (param->testflag & T_REP_ANY)
	(void) fprintf(stderr,
		     "Try fixing it by using the --safe-recover (-o), the --force (-f) option or by not using the --quick (-q) flag\n");
    }
    else if (!(param->error_printed & 2) &&
	     !(param->testflag & T_FORCE_CREATE))
      (void) fprintf(stderr,
      "MyISAM-table '%s' is corrupted\nFix it using switch \"-r\" or \"-o\"\n",
	      filename);
  }
  else if (param->warning_printed &&
	   ! (param->testflag & (T_REP_ANY | T_SORT_RECORDS | T_SORT_INDEX |
			  T_FORCE_CREATE)))
    (void) fprintf(stderr, "MyISAM-table '%s' is usable but should be fixed\n",
		 filename);
  (void) fflush(stderr);
  DBUG_RETURN(error);
} /* myisamchk */