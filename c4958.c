static void write_header(FILE *sql_file, char *db_name)
{
  if (opt_xml)
  {
    fputs("<?xml version=\"1.0\"?>\n", sql_file);
    /*
      Schema reference.  Allows use of xsi:nil for NULL values and 
      xsi:type to define an element's data type.
    */
    fputs("<mysqldump ", sql_file);
    fputs("xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"",
          sql_file);
    fputs(">\n", sql_file);
    check_io(sql_file);
  }
  else if (!opt_compact)
  {
    print_comment(sql_file, 0,
                  "-- MySQL dump %s  Distrib %s, for %s (%s)\n--\n",
                  DUMP_VERSION, MYSQL_SERVER_VERSION, SYSTEM_TYPE,
                  MACHINE_TYPE);
    print_comment(sql_file, 0, "-- Host: %s    Database: %s\n",
                  current_host ? current_host : "localhost",
                  db_name ? db_name : "");
    print_comment(sql_file, 0,
                  "-- ------------------------------------------------------\n"
                 );
    print_comment(sql_file, 0, "-- Server version\t%s\n",
                  mysql_get_server_info(&mysql_connection));

    if (opt_set_charset)
      fprintf(sql_file,
"\n/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;"
"\n/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;"
"\n/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;"
"\n/*!40101 SET NAMES %s */;\n",default_charset);

    if (opt_tz_utc)
    {
      fprintf(sql_file, "/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;\n");
      fprintf(sql_file, "/*!40103 SET TIME_ZONE='+00:00' */;\n");
    }

    if (!path)
    {
      fprintf(md_result_file,"\
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;\n\
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;\n\
");
    }
    fprintf(sql_file,
            "/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='%s%s%s' */;\n"
            "/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;\n",
            path?"":"NO_AUTO_VALUE_ON_ZERO",compatible_mode_normal_str[0]==0?"":",",
            compatible_mode_normal_str);
    check_io(sql_file);
  }
} /* write_header */