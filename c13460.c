std::shared_ptr<SQLiteDBInstance> getTestDBC() {
  auto dbc = SQLiteDBManager::getUnique();
  char* err = nullptr;
  std::vector<std::string> queries = {
      "CREATE TABLE test_table (username varchar(30) primary key, age int)",
      "INSERT INTO test_table VALUES (\"mike\", 23)",
      "INSERT INTO test_table VALUES (\"matt\", 24)"};

  for (auto q : queries) {
    sqlite3_exec(dbc->db(), q.c_str(), nullptr, nullptr, &err);
    if (err != nullptr) {
      throw std::domain_error(std::string("Cannot create testing DBC's db: ") +
                              err);
    }
  }

  return dbc;
}