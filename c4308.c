int mi_sort_index(MI_CHECK *param, register MI_INFO *info, char * name)
{
  reg2 uint key;
  reg1 MI_KEYDEF *keyinfo;
  File new_file;
  my_off_t index_pos[HA_MAX_POSSIBLE_KEY];
  uint r_locks,w_locks;
  int old_lock;
  MYISAM_SHARE *share=info->s;
  MI_STATE_INFO old_state;
  DBUG_ENTER("mi_sort_index");

  /* cannot sort index files with R-tree indexes */
  for (key= 0,keyinfo= &share->keyinfo[0]; key < share->base.keys ;
       key++,keyinfo++)
    if (keyinfo->key_alg == HA_KEY_ALG_RTREE)
      DBUG_RETURN(0);

  if (!(param->testflag & T_SILENT))
    printf("- Sorting index for MyISAM-table '%s'\n",name);

  /* Get real path for index file */
  fn_format(param->temp_filename,name,"", MI_NAME_IEXT,2+4+32);
  if ((new_file= mysql_file_create(mi_key_file_datatmp,
                                   fn_format(param->temp_filename,
                                             param->temp_filename,
                                             "", INDEX_TMP_EXT, 2+4),
                                   0, param->tmpfile_createflag, MYF(0))) <= 0)
  {
    mi_check_print_error(param,"Can't create new tempfile: '%s'",
			 param->temp_filename);
    DBUG_RETURN(-1);
  }
  if (filecopy(param, new_file,share->kfile,0L,
	       (ulong) share->base.keystart, "headerblock"))
    goto err;

  param->new_file_pos=share->base.keystart;
  for (key= 0,keyinfo= &share->keyinfo[0]; key < share->base.keys ;
       key++,keyinfo++)
  {
    if (! mi_is_key_active(info->s->state.key_map, key))
      continue;

    if (share->state.key_root[key] != HA_OFFSET_ERROR)
    {
      index_pos[key]=param->new_file_pos;	/* Write first block here */
      if (sort_one_index(param,info,keyinfo,share->state.key_root[key],
			 new_file))
	goto err;
    }
    else
      index_pos[key]= HA_OFFSET_ERROR;		/* No blocks */
  }

  /* Flush key cache for this file if we are calling this outside myisamchk */
  flush_key_blocks(share->key_cache,share->kfile, FLUSH_IGNORE_CHANGED);

  share->state.version=(ulong) time((time_t*) 0);
  old_state= share->state;			/* save state if not stored */
  r_locks=   share->r_locks;
  w_locks=   share->w_locks;
  old_lock=  info->lock_type;

	/* Put same locks as old file */
  share->r_locks= share->w_locks= share->tot_locks= 0;
  (void) _mi_writeinfo(info,WRITEINFO_UPDATE_KEYFILE);
  (void) mysql_file_close(share->kfile, MYF(MY_WME));
  share->kfile = -1;
  (void) mysql_file_close(new_file, MYF(MY_WME));
  if (change_to_newfile(share->index_file_name, MI_NAME_IEXT, INDEX_TMP_EXT,
			MYF(0)) ||
      mi_open_keyfile(share))
    goto err2;
  info->lock_type= F_UNLCK;			/* Force mi_readinfo to lock */
  _mi_readinfo(info,F_WRLCK,0);			/* Will lock the table */
  info->lock_type=  old_lock;
  share->r_locks=   r_locks;
  share->w_locks=   w_locks;
  share->tot_locks= r_locks+w_locks;
  share->state=     old_state;			/* Restore old state */

  info->state->key_file_length=param->new_file_pos;
  info->update= (short) (HA_STATE_CHANGED | HA_STATE_ROW_CHANGED);
  for (key=0 ; key < info->s->base.keys ; key++)
    info->s->state.key_root[key]=index_pos[key];
  for (key=0 ; key < info->s->state.header.max_block_size_index ; key++)
    info->s->state.key_del[key]=  HA_OFFSET_ERROR;

  info->s->state.changed&= ~STATE_NOT_SORTED_PAGES;
  DBUG_RETURN(0);

err:
  (void) mysql_file_close(new_file, MYF(MY_WME));
err2:
  (void) mysql_file_delete(mi_key_file_datatmp,
                           param->temp_filename, MYF(MY_WME));
  DBUG_RETURN(-1);
} /* mi_sort_index */