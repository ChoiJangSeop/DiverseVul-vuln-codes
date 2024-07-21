NamespaceString IDLParserErrorContext::parseNSCollectionRequired(StringData dbName,
                                                                 const BSONElement& element) {
    uassert(ErrorCodes::BadValue,
            str::stream() << "collection name has invalid type " << typeName(element.type()),
            element.canonicalType() == canonicalizeBSONType(mongo::String));

    const NamespaceString nss(dbName, element.valueStringData());

    uassert(ErrorCodes::InvalidNamespace,
            str::stream() << "Invalid namespace specified '" << nss.ns() << "'",
            nss.isValid());

    return nss;
}