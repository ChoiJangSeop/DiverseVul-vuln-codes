void MongoStatusInfo::postInstall(JSContext* cx, JS::HandleObject global, JS::HandleObject proto) {
    auto scope = getScope(cx);

    JS_SetPrivate(proto, scope->trackedNew<Status>(Status::OK()));
}