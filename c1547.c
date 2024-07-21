void PostgreSqlStorage::initDbSession(QSqlDatabase &db)
{
    // this blows... but unfortunately Qt's PG driver forces us to this...
    db.exec("set standard_conforming_strings = off");
    db.exec("set escape_string_warning = off");
}