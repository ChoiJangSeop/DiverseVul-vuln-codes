int mi_repair_parallel(MI_CHECK *param, register MI_INFO *info,
			const char * name, int rep_quick)
{
  int got_error;
  uint i,key, total_key_length, istep;
  ulong rec_length;
  ha_rows start_records;
  my_off_t new_header_length,del;
  File new_file;
  MI_SORT_PARAM *sort_param=0;
  MYISAM_SHARE *share=info->s;
  ulong   *rec_per_key_part;
  HA_KEYSEG *keyseg;
  char llbuff[22];
  IO_CACHE new_data_cache; /* For non-quick repair. */
  IO_CACHE_SHARE io_share;
  SORT_INFO sort_info;
  ulonglong UNINIT_VAR(key_map);
  pthread_attr_t thr_attr;
  ulong max_pack_reclength;
  int error;
  DBUG_ENTER("mi_repair_parallel");

  start_records=info->state->records;
  got_error=1;
  new_file= -1;
  new_header_length=(param->testflag & T_UNPACK) ? 0 :
    share->pack.header_length;
  if (!(param->testflag & T_SILENT))
  {
    printf("- parallel recovering (with sort) MyISAM-table '%s'\n",name);
    printf("Data records: %s\n", llstr(start_records,llbuff));
  }
  param->testflag|=T_REP; /* for easy checking */

  if (info->s->options & (HA_OPTION_CHECKSUM | HA_OPTION_COMPRESS_RECORD))
    param->testflag|=T_CALC_CHECKSUM;

  /*
    Quick repair (not touching data file, rebuilding indexes):
    {
      Read  cache is (MI_CHECK *param)->read_cache using info->dfile.
    }

    Non-quick repair (rebuilding data file and indexes):
    {
      Master thread:

        Read  cache is (MI_CHECK *param)->read_cache using info->dfile.
        Write cache is (MI_INFO   *info)->rec_cache  using new_file.

      Slave threads:

        Read  cache is new_data_cache synced to master rec_cache.

      The final assignment of the filedescriptor for rec_cache is done
      after the cache creation.

      Don't check file size on new_data_cache, as the resulting file size
      is not known yet.

      As rec_cache and new_data_cache are synced, write_buffer_length is
      used for the read cache 'new_data_cache'. Both start at the same
      position 'new_header_length'.
    }
  */
  DBUG_PRINT("info", ("is quick repair: %d", rep_quick));
  bzero((char*)&sort_info,sizeof(sort_info));
  /* Initialize pthread structures before goto err. */
  mysql_mutex_init(mi_key_mutex_MI_SORT_INFO_mutex,
                   &sort_info.mutex, MY_MUTEX_INIT_FAST);
  mysql_cond_init(mi_key_cond_MI_SORT_INFO_cond, &sort_info.cond, 0);
  mysql_mutex_init(mi_key_mutex_MI_CHECK_print_msg,
                   &param->print_msg_mutex, MY_MUTEX_INIT_FAST);
  param->need_print_msg_lock= 1;

  if (!(sort_info.key_block=
	alloc_key_blocks(param, (uint) param->sort_key_blocks,
			 share->base.max_key_block_length)) ||
      init_io_cache(&param->read_cache, info->dfile,
                    (uint) param->read_buffer_length,
                    READ_CACHE, share->pack.header_length, 1, MYF(MY_WME)) ||
      (!rep_quick &&
       (init_io_cache(&info->rec_cache, info->dfile,
                      (uint) param->write_buffer_length,
                      WRITE_CACHE, new_header_length, 1,
                      MYF(MY_WME | MY_WAIT_IF_FULL) & param->myf_rw) ||
        init_io_cache(&new_data_cache, -1,
                      (uint) param->write_buffer_length,
                      READ_CACHE, new_header_length, 1,
                      MYF(MY_WME | MY_DONT_CHECK_FILESIZE)))))
    goto err;
  sort_info.key_block_end=sort_info.key_block+param->sort_key_blocks;
  info->opt_flag|=WRITE_CACHE_USED;
  info->rec_cache.file=info->dfile;         /* for sort_delete_record */

  if (!rep_quick)
  {
    /* Get real path for data file */
    if ((new_file= mysql_file_create(mi_key_file_datatmp,
                                     fn_format(param->temp_filename,
                                               share->data_file_name, "",
                                               DATA_TMP_EXT, 2+4),
                                     0, param->tmpfile_createflag,
                                     MYF(0))) < 0)
    {
      mi_check_print_error(param,"Can't create new tempfile: '%s'",
			   param->temp_filename);
      goto err;
    }
    if (new_header_length &&
        filecopy(param, new_file,info->dfile,0L,new_header_length,
		 "datafile-header"))
      goto err;
    if (param->testflag & T_UNPACK)
    {
      share->options&= ~HA_OPTION_COMPRESS_RECORD;
      mi_int2store(share->state.header.options,share->options);
    }
    share->state.dellink= HA_OFFSET_ERROR;
    info->rec_cache.file=new_file;
  }

  info->update= (short) (HA_STATE_CHANGED | HA_STATE_ROW_CHANGED);

  /* Optionally drop indexes and optionally modify the key_map. */
  mi_drop_all_indexes(param, info, FALSE);
  key_map= share->state.key_map;
  if (param->testflag & T_CREATE_MISSING_KEYS)
  {
    /* Invert the copied key_map to recreate all disabled indexes. */
    key_map= ~key_map;
  }

  sort_info.info=info;
  sort_info.param = param;

  set_data_file_type(&sort_info, share);
  sort_info.dupp=0;
  sort_info.buff=0;
  param->read_cache.end_of_file=sort_info.filelength=
    mysql_file_seek(param->read_cache.file, 0L, MY_SEEK_END, MYF(0));

  if (share->data_file_type == DYNAMIC_RECORD)
    rec_length=max(share->base.min_pack_length+1,share->base.min_block_length);
  else if (share->data_file_type == COMPRESSED_RECORD)
    rec_length=share->base.min_block_length;
  else
    rec_length=share->base.pack_reclength;
  /*
    +1 below is required hack for parallel repair mode.
    The info->state->records value, that is compared later
    to sort_info.max_records and cannot exceed it, is
    increased in sort_key_write. In mi_repair_by_sort, sort_key_write
    is called after sort_key_read, where the comparison is performed,
    but in parallel mode master thread can call sort_key_write
    before some other repair thread calls sort_key_read.
    Furthermore I'm not even sure +1 would be enough.
    May be sort_info.max_records shold be always set to max value in
    parallel mode.
  */
  sort_info.max_records=
    ((param->testflag & T_CREATE_MISSING_KEYS) ? info->state->records + 1:
     (ha_rows) (sort_info.filelength/rec_length+1));

  del=info->state->del;
  param->glob_crc=0;
  /* for compressed tables */
  max_pack_reclength= share->base.pack_reclength;
  if (share->options & HA_OPTION_COMPRESS_RECORD)
    set_if_bigger(max_pack_reclength, share->max_pack_length);
  if (!(sort_param=(MI_SORT_PARAM *)
        my_malloc((uint) share->base.keys *
		  (sizeof(MI_SORT_PARAM) + max_pack_reclength),
		  MYF(MY_ZEROFILL))))
  {
    mi_check_print_error(param,"Not enough memory for key!");
    goto err;
  }
  total_key_length=0;
  rec_per_key_part= param->rec_per_key_part;
  info->state->records=info->state->del=share->state.split=0;
  info->state->empty=0;

  for (i=key=0, istep=1 ; key < share->base.keys ;
       rec_per_key_part+=sort_param[i].keyinfo->keysegs, i+=istep, key++)
  {
    sort_param[i].key=key;
    sort_param[i].keyinfo=share->keyinfo+key;
    sort_param[i].seg=sort_param[i].keyinfo->seg;
    /*
      Skip this index if it is marked disabled in the copied
      (and possibly inverted) key_map.
    */
    if (! mi_is_key_active(key_map, key))
    {
      /* Remember old statistics for key */
      memcpy((char*) rec_per_key_part,
	     (char*) (share->state.rec_per_key_part+
		      (uint) (rec_per_key_part - param->rec_per_key_part)),
	     sort_param[i].keyinfo->keysegs*sizeof(*rec_per_key_part));
      istep=0;
      continue;
    }
    istep=1;
    if ((!(param->testflag & T_SILENT)))
      printf ("- Fixing index %d\n",key+1);
    if (sort_param[i].keyinfo->flag & HA_FULLTEXT)
    {
      sort_param[i].key_read=sort_ft_key_read;
      sort_param[i].key_write=sort_ft_key_write;
    }
    else
    {
      sort_param[i].key_read=sort_key_read;
      sort_param[i].key_write=sort_key_write;
    }
    sort_param[i].key_cmp=sort_key_cmp;
    sort_param[i].lock_in_memory=lock_memory;
    sort_param[i].tmpdir=param->tmpdir;
    sort_param[i].sort_info=&sort_info;
    sort_param[i].master=0;
    sort_param[i].fix_datafile=0;
    sort_param[i].calc_checksum= 0;

    sort_param[i].filepos=new_header_length;
    sort_param[i].max_pos=sort_param[i].pos=share->pack.header_length;

    sort_param[i].record= (((uchar *)(sort_param+share->base.keys))+
			   (max_pack_reclength * i));
    if (!mi_alloc_rec_buff(info, -1, &sort_param[i].rec_buff))
    {
      mi_check_print_error(param,"Not enough memory!");
      goto err;
    }

    sort_param[i].key_length=share->rec_reflength;
    for (keyseg=sort_param[i].seg; keyseg->type != HA_KEYTYPE_END;
	 keyseg++)
    {
      sort_param[i].key_length+=keyseg->length;
      if (keyseg->flag & HA_SPACE_PACK)
        sort_param[i].key_length+=get_pack_length(keyseg->length);
      if (keyseg->flag & (HA_BLOB_PART | HA_VAR_LENGTH_PART))
        sort_param[i].key_length+=2 + test(keyseg->length >= 127);
      if (keyseg->flag & HA_NULL_PART)
        sort_param[i].key_length++;
    }
    total_key_length+=sort_param[i].key_length;

    if (sort_param[i].keyinfo->flag & HA_FULLTEXT)
    {
      uint ft_max_word_len_for_sort=FT_MAX_WORD_LEN_FOR_SORT*
                                    sort_param[i].keyinfo->seg->charset->mbmaxlen;
      sort_param[i].key_length+=ft_max_word_len_for_sort-HA_FT_MAXBYTELEN;
      init_alloc_root(&sort_param[i].wordroot, FTPARSER_MEMROOT_ALLOC_SIZE, 0);
    }
  }
  sort_info.total_keys=i;
  sort_param[0].master= 1;
  sort_param[0].fix_datafile= (my_bool)(! rep_quick);
  sort_param[0].calc_checksum= test(param->testflag & T_CALC_CHECKSUM);

  if (!ftparser_alloc_param(info))
    goto err;

  sort_info.got_error=0;
  mysql_mutex_lock(&sort_info.mutex);

  /*
    Initialize the I/O cache share for use with the read caches and, in
    case of non-quick repair, the write cache. When all threads join on
    the cache lock, the writer copies the write cache contents to the
    read caches.
  */
  if (i > 1)
  {
    if (rep_quick)
      init_io_cache_share(&param->read_cache, &io_share, NULL, i);
    else
      init_io_cache_share(&new_data_cache, &io_share, &info->rec_cache, i);
  }
  else
    io_share.total_threads= 0; /* share not used */

  (void) pthread_attr_init(&thr_attr);
  (void) pthread_attr_setdetachstate(&thr_attr,PTHREAD_CREATE_DETACHED);

  for (i=0 ; i < sort_info.total_keys ; i++)
  {
    /*
      Copy the properly initialized IO_CACHE structure so that every
      thread has its own copy. In quick mode param->read_cache is shared
      for use by all threads. In non-quick mode all threads but the
      first copy the shared new_data_cache, which is synchronized to the
      write cache of the first thread. The first thread copies
      param->read_cache, which is not shared.
    */
    sort_param[i].read_cache= ((rep_quick || !i) ? param->read_cache :
                               new_data_cache);
    DBUG_PRINT("io_cache_share", ("thread: %u  read_cache: 0x%lx",
                                  i, (long) &sort_param[i].read_cache));

    /*
      two approaches: the same amount of memory for each thread
      or the memory for the same number of keys for each thread...
      In the second one all the threads will fill their sort_buffers
      (and call write_keys) at the same time, putting more stress on i/o.
    */
    sort_param[i].sortbuff_size=
#ifndef USING_SECOND_APPROACH
      param->sort_buffer_length/sort_info.total_keys;
#else
      param->sort_buffer_length*sort_param[i].key_length/total_key_length;
#endif
    if ((error= mysql_thread_create(mi_key_thread_find_all_keys,
                                    &sort_param[i].thr, &thr_attr,
                                    thr_find_all_keys,
                                    (void *) (sort_param+i))))
    {
      mi_check_print_error(param,"Cannot start a repair thread (errno= %d)",
                           error);
      /* Cleanup: Detach from the share. Avoid others to be blocked. */
      if (io_share.total_threads)
        remove_io_thread(&sort_param[i].read_cache);
      DBUG_PRINT("error", ("Cannot start a repair thread"));
      sort_info.got_error=1;
    }
    else
      sort_info.threads_running++;
  }
  (void) pthread_attr_destroy(&thr_attr);

  /* waiting for all threads to finish */
  while (sort_info.threads_running)
    mysql_cond_wait(&sort_info.cond, &sort_info.mutex);
  mysql_mutex_unlock(&sort_info.mutex);

  if ((got_error= thr_write_keys(sort_param)))
  {
    param->retry_repair=1;
    goto err;
  }
  got_error=1;				/* Assume the following may go wrong */

  if (sort_param[0].fix_datafile)
  {
    /*
      Append some nuls to the end of a memory mapped file. Destroy the
      write cache. The master thread did already detach from the share
      by remove_io_thread() in sort.c:thr_find_all_keys().
    */
    if (write_data_suffix(&sort_info,1) || end_io_cache(&info->rec_cache))
      goto err;
    if (param->testflag & T_SAFE_REPAIR)
    {
      /* Don't repair if we loosed more than one row */
      if (info->state->records+1 < start_records)
      {
        info->state->records=start_records;
        goto err;
      }
    }
    share->state.state.data_file_length= info->state->data_file_length=
      sort_param->filepos;
    /* Only whole records */
    share->state.version=(ulong) time((time_t*) 0);

    /*
      Exchange the data file descriptor of the table, so that we use the
      new file from now on.
     */
    mysql_file_close(info->dfile, MYF(0));
    info->dfile=new_file;

    share->data_file_type=sort_info.new_data_file_type;
    share->pack.header_length=(ulong) new_header_length;
  }
  else
    info->state->data_file_length=sort_param->max_pos;

  if (rep_quick && del+sort_info.dupp != info->state->del)
  {
    mi_check_print_error(param,"Couldn't fix table with quick recovery: Found wrong number of deleted records");
    mi_check_print_error(param,"Run recovery again without -q");
    param->retry_repair=1;
    param->testflag|=T_RETRY_WITHOUT_QUICK;
    goto err;
  }

  if (rep_quick & T_FORCE_UNIQUENESS)
  {
    my_off_t skr=info->state->data_file_length+
      (share->options & HA_OPTION_COMPRESS_RECORD ?
       MEMMAP_EXTRA_MARGIN : 0);
#ifdef USE_RELOC
    if (share->data_file_type == STATIC_RECORD &&
	skr < share->base.reloc*share->base.min_pack_length)
      skr=share->base.reloc*share->base.min_pack_length;
#endif
    if (skr != sort_info.filelength)
      if (mysql_file_chsize(info->dfile, skr, 0, MYF(0)))
	mi_check_print_warning(param,
			       "Can't change size of datafile,  error: %d",
			       my_errno);
  }
  if (param->testflag & T_CALC_CHECKSUM)
    info->state->checksum=param->glob_crc;

  if (mysql_file_chsize(share->kfile, info->state->key_file_length, 0, MYF(0)))
    mi_check_print_warning(param,
			   "Can't change size of indexfile, error: %d", my_errno);

  if (!(param->testflag & T_SILENT))
  {
    if (start_records != info->state->records)
      printf("Data records: %s\n", llstr(info->state->records,llbuff));
    if (sort_info.dupp)
      mi_check_print_warning(param,
			     "%s records have been removed",
			     llstr(sort_info.dupp,llbuff));
  }
  got_error=0;

  if (&share->state.state != info->state)
    memcpy(&share->state.state, info->state, sizeof(*info->state));

err:
  got_error|= flush_blocks(param, share->key_cache, share->kfile);
  /*
    Destroy the write cache. The master thread did already detach from
    the share by remove_io_thread() or it was not yet started (if the
    error happend before creating the thread).
  */
  (void) end_io_cache(&info->rec_cache);
  /*
    Destroy the new data cache in case of non-quick repair. All slave
    threads did either detach from the share by remove_io_thread()
    already or they were not yet started (if the error happend before
    creating the threads).
  */
  if (!rep_quick)
    (void) end_io_cache(&new_data_cache);
  if (!got_error)
  {
    /* Replace the actual file with the temporary file */
    if (new_file >= 0)
    {
      mysql_file_close(new_file, MYF(0));
      info->dfile=new_file= -1;
      if (change_to_newfile(share->data_file_name, MI_NAME_DEXT, DATA_TMP_EXT,
			    (param->testflag & T_BACKUP_DATA ?
			     MYF(MY_REDEL_MAKE_BACKUP): MYF(0))) ||
	  mi_open_datafile(info,share,name,-1))
	got_error=1;
    }
  }
  if (got_error)
  {
    if (! param->error_printed)
      mi_check_print_error(param,"%d when fixing table",my_errno);
    if (new_file >= 0)
    {
      (void) mysql_file_close(new_file, MYF(0));
      (void) mysql_file_delete(mi_key_file_datatmp,
                               param->temp_filename, MYF(MY_WME));
      if (info->dfile == new_file) /* Retry with key cache */
        if (unlikely(mi_open_datafile(info, share, name, -1)))
          param->retry_repair= 0; /* Safety */
    }
    mi_mark_crashed_on_repair(info);
  }
  else if (key_map == share->state.key_map)
    share->state.changed&= ~STATE_NOT_OPTIMIZED_KEYS;
  share->state.changed|=STATE_NOT_SORTED_PAGES;

  mysql_cond_destroy(&sort_info.cond);
  mysql_mutex_destroy(&sort_info.mutex);
  mysql_mutex_destroy(&param->print_msg_mutex);
  param->need_print_msg_lock= 0;

  my_free(sort_info.ft_buf);
  my_free(sort_info.key_block);
  my_free(sort_param);
  my_free(sort_info.buff);
  (void) end_io_cache(&param->read_cache);
  info->opt_flag&= ~(READ_CACHE_USED | WRITE_CACHE_USED);
  if (!got_error && (param->testflag & T_UNPACK))
  {
    share->state.header.options[0]&= (uchar) ~HA_OPTION_COMPRESS_RECORD;
    share->pack.header_length=0;
  }
  DBUG_RETURN(got_error);
}