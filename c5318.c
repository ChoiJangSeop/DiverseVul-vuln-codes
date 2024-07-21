my_bool STDCALL mysql_stmt_close(MYSQL_STMT *stmt)
{
  MYSQL *mysql= stmt->mysql;
  int rc= 0;
  DBUG_ENTER("mysql_stmt_close");

  free_root(&stmt->result.alloc, MYF(0));
  free_root(&stmt->mem_root, MYF(0));
  free_root(&stmt->extension->fields_mem_root, MYF(0));

  if (mysql)
  {
    mysql->stmts= list_delete(mysql->stmts, &stmt->list);
    /*
      Clear NET error state: if the following commands come through
      successfully, connection will still be usable for other commands.
    */
    net_clear_error(&mysql->net);
    if ((int) stmt->state > (int) MYSQL_STMT_INIT_DONE)
    {
      uchar buff[MYSQL_STMT_HEADER];             /* 4 bytes - stmt id */

      if (mysql->unbuffered_fetch_owner == &stmt->unbuffered_fetch_cancelled)
        mysql->unbuffered_fetch_owner= 0;
      if (mysql->status != MYSQL_STATUS_READY)
      {
        /*
          Flush result set of the connection. If it does not belong
          to this statement, set a warning.
        */
        (*mysql->methods->flush_use_result)(mysql, TRUE);
        if (mysql->unbuffered_fetch_owner)
          *mysql->unbuffered_fetch_owner= TRUE;
        mysql->status= MYSQL_STATUS_READY;
      }
      int4store(buff, stmt->stmt_id);
      if ((rc= stmt_command(mysql, COM_STMT_CLOSE, buff, 4, stmt)))
      {
        set_stmt_errmsg(stmt, &mysql->net);
      }
    }
  }

  my_free(stmt->extension);
  my_free(stmt);

  DBUG_RETURN(test(rc));
}