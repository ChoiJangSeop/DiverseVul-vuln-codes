static void usage(void)
{
  PRINT_VERSION;
  puts("Copyright (c) 2011, Oracle and/or its affiliates. "
       "All rights reserved.\n");
  puts("Enable or disable plugins.");
  printf("\nUsage: %s [options] <plugin> ENABLE|DISABLE\n\nOptions:\n",
     my_progname);
  my_print_help(my_long_options);
  puts("\n");
}