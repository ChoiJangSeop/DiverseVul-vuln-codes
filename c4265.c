static void reopen_slow_log(char* name)
{
  logger.get_slow_log_file_handler()->close(0);
  logger.get_slow_log_file_handler()->open_slow_log(name);
}