static void reopen_general_log(char* name)
{
  logger.get_log_file_handler()->close(0);
  logger.get_log_file_handler()->open_query_log(name);
}