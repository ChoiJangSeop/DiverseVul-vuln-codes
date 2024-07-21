init_syntax_once ()
{
   register int c;
   static int done;

   if (done)
     return;

   bzero (re_syntax_table, sizeof re_syntax_table);

   for (c = 'a'; c <= 'z'; c++)
     re_syntax_table[c] = Sword;

   for (c = 'A'; c <= 'Z'; c++)
     re_syntax_table[c] = Sword;

   for (c = '0'; c <= '9'; c++)
     re_syntax_table[c] = Sword;

   re_syntax_table['_'] = Sword;

   done = 1;
}