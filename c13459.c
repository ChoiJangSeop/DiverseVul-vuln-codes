Status genTableRowsForSqliteTable(const fs::path& sqlite_db,
                                  const std::string& sqlite_query,
                                  TableRows& results,
                                  bool respect_locking) {
  sqlite3* db = nullptr;
  if (!pathExists(sqlite_db).ok()) {
    return Status(1, "Database path does not exist");
  }

  auto rc = sqlite3_open_v2(
      sqlite_db.string().c_str(),
      &db,
      (SQLITE_OPEN_READONLY | SQLITE_OPEN_PRIVATECACHE | SQLITE_OPEN_NOMUTEX),
      getSystemVFS(respect_locking));
  if (rc != SQLITE_OK || db == nullptr) {
    VLOG(1) << "Cannot open specified database: "
            << getStringForSQLiteReturnCode(rc);
    if (db != nullptr) {
      sqlite3_close(db);
    }
    return Status(1, "Could not open database");
  }

  sqlite3_stmt* stmt = nullptr;
  rc = sqlite3_prepare_v2(db, sqlite_query.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    sqlite3_close(db);
    VLOG(1) << "ATC table: Could not prepare database at path: " << sqlite_db;
    return Status(rc, "Could not prepare database");
  }

  while ((sqlite3_step(stmt)) == SQLITE_ROW) {
    auto s = genSqliteTableRow(stmt, results, sqlite_db);
    if (!s.ok()) {
      break;
    }
  }

  // Close handles and free memory
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  return Status{};
}