void MongoStatusInfo::fromStatus(JSContext* cx, Status status, JS::MutableHandleValue value) {
    auto scope = getScope(cx);

    JS::RootedValue undef(cx);
    undef.setUndefined();

    JS::AutoValueArray<1> args(cx);
    ValueReader(cx, args[0]).fromStringData(status.reason());
    JS::RootedObject error(cx);
    scope->getProto<ErrorInfo>().newInstance(args, &error);

    JS::RootedObject thisv(cx);
    scope->getProto<MongoStatusInfo>().newObjectWithProto(&thisv, error);
    ObjectWrapper thisvObj(cx, thisv);
    thisvObj.defineProperty(InternedString::code,
                            JSPROP_ENUMERATE,
                            smUtils::wrapConstrainedMethod<Functions::code, false, MongoStatusInfo>,
                            nullptr);

    thisvObj.defineProperty(
        InternedString::reason,
        JSPROP_ENUMERATE,
        smUtils::wrapConstrainedMethod<Functions::reason, false, MongoStatusInfo>,
        nullptr);

    // We intentionally omit JSPROP_ENUMERATE to match how Error.prototype.stack is a non-enumerable
    // property.
    thisvObj.defineProperty(
        InternedString::stack,
        0,
        smUtils::wrapConstrainedMethod<Functions::stack, false, MongoStatusInfo>,
        nullptr);

    JS_SetPrivate(thisv, scope->trackedNew<Status>(std::move(status)));

    value.setObjectOrNull(thisv);
}