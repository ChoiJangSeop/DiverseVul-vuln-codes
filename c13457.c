SQLiteDBInstanceRef SQLiteDBManager::getConnection(bool primary) {
  auto& self = instance();
  WriteLock lock(self.create_mutex_);

  if (self.db_ == nullptr) {
    // Create primary SQLite DB instance.
    openOptimized(self.db_);
    self.connection_ = SQLiteDBInstanceRef(new SQLiteDBInstance(self.db_));
    attachVirtualTables(self.connection_);
  }

  // Internal usage may request the primary connection explicitly.
  if (primary) {
    return self.connection_;
  }

  // Create a 'database connection' for the managed database instance.
  auto instance = std::make_shared<SQLiteDBInstance>(self.db_, self.mutex_);
  if (!instance->isPrimary()) {
    attachVirtualTables(instance);
  }
  return instance;
}