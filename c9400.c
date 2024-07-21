read_points(const char *filename)
{
  FILE *f;
  char line[256];
  struct Point *p;

  f = fopen(filename, "r");
  if (!f) {
    LOG_FATAL("Could not open tempcomp point file %s", filename);
    return;
  }

  points = ARR_CreateInstance(sizeof (struct Point));

  while (fgets(line, sizeof (line), f)) {
    p = (struct Point *)ARR_GetNewElement(points);
    if (sscanf(line, "%lf %lf", &p->temp, &p->comp) != 2) {
      LOG_FATAL("Could not read tempcomp point from %s", filename);
      break;
    }
  }

  fclose(f);

  if (ARR_GetSize(points) < 2)
    LOG_FATAL("Not enough points in %s", filename);
}