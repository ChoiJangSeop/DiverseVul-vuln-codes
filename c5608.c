int ha_partition::del_ren_cre_table(const char *from,
				     const char *to,
				     TABLE *table_arg,
				     HA_CREATE_INFO *create_info)
{
  int save_error= 0;
  int error= HA_ERR_INTERNAL_ERROR;
  char from_buff[FN_REFLEN], to_buff[FN_REFLEN], from_lc_buff[FN_REFLEN],
       to_lc_buff[FN_REFLEN], buff[FN_REFLEN];
  char *name_buffer_ptr;
  const char *from_path;
  const char *to_path= NULL;
  uint i;
  handler **file, **abort_file;
  DBUG_ENTER("del_ren_cre_table()");

  /* Not allowed to create temporary partitioned tables */
  if (create_info && create_info->options & HA_LEX_CREATE_TMP_TABLE)
  {
    my_error(ER_PARTITION_NO_TEMPORARY, MYF(0));
    DBUG_RETURN(error);
  }

  fn_format(buff,from, "", ha_par_ext, MY_APPEND_EXT);
  /* Check if the  par file exists */
  if (my_access(buff,F_OK))
  {
    /*
      If the .par file does not exist, return HA_ERR_NO_SUCH_TABLE,
      This will signal to the caller that it can remove the .frm
      file.
    */
    error= HA_ERR_NO_SUCH_TABLE;
    DBUG_RETURN(error);
  }

  if (get_from_handler_file(from, ha_thd()->mem_root, false))
    DBUG_RETURN(error);
  DBUG_ASSERT(m_file_buffer);
  DBUG_PRINT("enter", ("from: (%s) to: (%s)", from, to ? to : "(nil)"));
  name_buffer_ptr= m_name_buffer_ptr;
  file= m_file;
  /*
    Since ha_partition has HA_FILE_BASED, it must alter underlying table names
    if they do not have HA_FILE_BASED and lower_case_table_names == 2.
    See Bug#37402, for Mac OS X.
    The appended #P#<partname>[#SP#<subpartname>] will remain in current case.
    Using the first partitions handler, since mixing handlers is not allowed.
  */
  from_path= get_canonical_filename(*file, from, from_lc_buff);
  if (to != NULL)
    to_path= get_canonical_filename(*file, to, to_lc_buff);
  i= 0;
  do
  {
    create_partition_name(from_buff, from_path, name_buffer_ptr,
                          NORMAL_PART_NAME, FALSE);

    if (to != NULL)
    {						// Rename branch
      create_partition_name(to_buff, to_path, name_buffer_ptr,
                            NORMAL_PART_NAME, FALSE);
      error= (*file)->ha_rename_table(from_buff, to_buff);
      if (error)
        goto rename_error;
    }
    else if (table_arg == NULL)			// delete branch
      error= (*file)->ha_delete_table(from_buff);
    else
    {
      if ((error= set_up_table_before_create(table_arg, from_buff,
                                             create_info, i, NULL)) ||
          ((error= (*file)->ha_create(from_buff, table_arg, create_info))))
        goto create_error;
    }
    name_buffer_ptr= strend(name_buffer_ptr) + 1;
    if (error)
      save_error= error;
    i++;
  } while (*(++file));

  if (to == NULL && table_arg == NULL)
  {
    DBUG_EXECUTE_IF("crash_before_deleting_par_file", DBUG_SUICIDE(););

    /* Delete the .par file. If error, break.*/
    if ((error= handler::delete_table(from)))
      DBUG_RETURN(error);

    DBUG_EXECUTE_IF("crash_after_deleting_par_file", DBUG_SUICIDE(););
  }

  if (to != NULL)
  {
    if ((error= handler::rename_table(from, to)))
    {
      /* Try to revert everything, ignore errors */
      (void) handler::rename_table(to, from);
      goto rename_error;
    }
  }
  DBUG_RETURN(save_error);
create_error:
  name_buffer_ptr= m_name_buffer_ptr;
  for (abort_file= file, file= m_file; file < abort_file; file++)
  {
    create_partition_name(from_buff, from_path, name_buffer_ptr, NORMAL_PART_NAME,
                          FALSE);
    (void) (*file)->ha_delete_table((const char*) from_buff);
    name_buffer_ptr= strend(name_buffer_ptr) + 1;
  }
  DBUG_RETURN(error);
rename_error:
  name_buffer_ptr= m_name_buffer_ptr;
  for (abort_file= file, file= m_file; file < abort_file; file++)
  {
    /* Revert the rename, back from 'to' to the original 'from' */
    create_partition_name(from_buff, from_path, name_buffer_ptr,
                          NORMAL_PART_NAME, FALSE);
    create_partition_name(to_buff, to_path, name_buffer_ptr,
                          NORMAL_PART_NAME, FALSE);
    /* Ignore error here */
    (void) (*file)->ha_rename_table(to_buff, from_buff);
    name_buffer_ptr= strend(name_buffer_ptr) + 1;
  }
  DBUG_RETURN(error);
}