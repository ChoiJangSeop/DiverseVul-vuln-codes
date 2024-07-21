NamespaceString Command::parseNsCollectionRequired(const string& dbname, const BSONObj& cmdObj) {
    // Accepts both BSON String and Symbol for collection name per SERVER-16260
    // TODO(kangas) remove Symbol support in MongoDB 3.0 after Ruby driver audit
    BSONElement first = cmdObj.firstElement();
    uassert(ErrorCodes::InvalidNamespace,
            str::stream() << "collection name has invalid type " << typeName(first.type()),
            first.canonicalType() == canonicalizeBSONType(mongo::String));
    const NamespaceString nss(dbname, first.valueStringData());
    uassert(ErrorCodes::InvalidNamespace,
            str::stream() << "Invalid namespace specified '" << nss.ns() << "'",
            nss.isValid());
    return nss;
}