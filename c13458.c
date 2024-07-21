TEST_F(SQLiteUtilTests, test_column_type_determination) {
  // Correct identification of text and ints
  testTypesExpected("select path, inode from file where path like '%'",
                    TypeMap({{"path", TEXT_TYPE}, {"inode", INTEGER_TYPE}}));
  // Correctly treating BLOBs as text
  testTypesExpected("select CAST(seconds AS BLOB) as seconds FROM time",
                    TypeMap({{"seconds", TEXT_TYPE}}));
  // Correctly treating ints cast as double as doubles
  testTypesExpected("select CAST(seconds AS DOUBLE) as seconds FROM time",
                    TypeMap({{"seconds", DOUBLE_TYPE}}));
  // Correctly treating bools as ints
  testTypesExpected("select CAST(seconds AS BOOLEAN) as seconds FROM time",
                    TypeMap({{"seconds", INTEGER_TYPE}}));
  // Correctly recognizing values from columns declared double as double, even
  // if they happen to have integer value.  And also test multi-statement
  // queries.
  testTypesExpected(
      "CREATE TABLE test_types_table (username varchar(30) primary key, age "
      "double);INSERT INTO test_types_table VALUES (\"mike\", 23); SELECT age "
      "from test_types_table",
      TypeMap({{"age", DOUBLE_TYPE}}));
}