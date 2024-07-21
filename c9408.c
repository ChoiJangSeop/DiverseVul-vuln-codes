CNF_ReadFile(const char *filename)
{
  FILE *in;
  char line[2048];
  int i;

  in = fopen(filename, "r");
  if (!in) {
    LOG_FATAL("Could not open configuration file %s : %s",
              filename, strerror(errno));
    return;
  }

  DEBUG_LOG("Reading %s", filename);

  for (i = 1; fgets(line, sizeof(line), in); i++) {
    CNF_ParseLine(filename, i, line);
  }

  fclose(in);
}