int my_redel(const char *org_name, const char *tmp_name,
             time_t backup_time_stamp, myf MyFlags)
{
  int error=1;
  DBUG_ENTER("my_redel");
  DBUG_PRINT("my",("org_name: '%s' tmp_name: '%s'  MyFlags: %d",
		   org_name,tmp_name,MyFlags));

  if (my_copystat(org_name,tmp_name,MyFlags) < 0)
    goto end;
  if (MyFlags & MY_REDEL_MAKE_BACKUP)
  {
    char name_buff[FN_REFLEN + MY_BACKUP_NAME_EXTRA_LENGTH];    
    my_create_backup_name(name_buff, org_name, backup_time_stamp);
    if (my_rename(org_name, name_buff, MyFlags))
      goto end;
  }
  else if (my_delete(org_name, MyFlags))
      goto end;
  if (my_rename(tmp_name,org_name,MyFlags))
    goto end;

  error=0;
end:
  DBUG_RETURN(error);
} /* my_redel */