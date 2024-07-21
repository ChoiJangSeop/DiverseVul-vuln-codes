void MongoStatusInfo::construct(JSContext* cx, JS::CallArgs args) {
    auto code = args.get(0).toInt32();
    auto reason = JSStringWrapper(cx, args.get(1).toString()).toString();

    JS::RootedValue out(cx);
    fromStatus(cx, Status(ErrorCodes::Error(code), std::move(reason)), &out);

    args.rval().set(out);
}