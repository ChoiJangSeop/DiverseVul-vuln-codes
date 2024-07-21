int FUZ_SecurityTest()
{
  char* output;
  char* input;
  int i, r;

  printf("Overflow test (issue 52)...\n");
  input = (char*) malloc (20<<20);
  output = (char*) malloc (20<<20);
  input[0] = 0x0F;
  input[1] = 0x00;
  input[2] = 0x00;
  for(i = 3; i < 16840000; i++)
    input[i] = 0xff;
  r = LZ4_decompress_fast(input, output, 20<<20);

  free(input);
  free(output);
  printf(" Passed (return = %i < 0)\n",r);
  return 0;
}