void InitializeHttpParser(Local<Object> target,
                          Local<Value> unused,
                          Local<Context> context,
                          void* priv) {
  Environment* env = Environment::GetCurrent(context);
  BindingData* const binding_data =
      env->AddBindingData<BindingData>(context, target);
  if (binding_data == nullptr) return;

  Local<FunctionTemplate> t = env->NewFunctionTemplate(Parser::New);
  t->InstanceTemplate()->SetInternalFieldCount(Parser::kInternalFieldCount);
  t->SetClassName(FIXED_ONE_BYTE_STRING(env->isolate(), "HTTPParser"));

  t->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "REQUEST"),
         Integer::New(env->isolate(), HTTP_REQUEST));
  t->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "RESPONSE"),
         Integer::New(env->isolate(), HTTP_RESPONSE));
  t->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "kOnHeaders"),
         Integer::NewFromUnsigned(env->isolate(), kOnHeaders));
  t->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "kOnHeadersComplete"),
         Integer::NewFromUnsigned(env->isolate(), kOnHeadersComplete));
  t->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "kOnBody"),
         Integer::NewFromUnsigned(env->isolate(), kOnBody));
  t->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "kOnMessageComplete"),
         Integer::NewFromUnsigned(env->isolate(), kOnMessageComplete));
  t->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "kOnExecute"),
         Integer::NewFromUnsigned(env->isolate(), kOnExecute));
  t->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "kOnTimeout"),
         Integer::NewFromUnsigned(env->isolate(), kOnTimeout));

  Local<Array> methods = Array::New(env->isolate());
#define V(num, name, string)                                                  \
    methods->Set(env->context(),                                              \
        num, FIXED_ONE_BYTE_STRING(env->isolate(), #string)).Check();
  HTTP_METHOD_MAP(V)
#undef V
  target->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "methods"),
              methods).Check();

  t->Inherit(AsyncWrap::GetConstructorTemplate(env));
  env->SetProtoMethod(t, "close", Parser::Close);
  env->SetProtoMethod(t, "free", Parser::Free);
  env->SetProtoMethod(t, "execute", Parser::Execute);
  env->SetProtoMethod(t, "finish", Parser::Finish);
  env->SetProtoMethod(t, "initialize", Parser::Initialize);
  env->SetProtoMethod(t, "pause", Parser::Pause<true>);
  env->SetProtoMethod(t, "resume", Parser::Pause<false>);
  env->SetProtoMethod(t, "consume", Parser::Consume);
  env->SetProtoMethod(t, "unconsume", Parser::Unconsume);
  env->SetProtoMethod(t, "getCurrentBuffer", Parser::GetCurrentBuffer);

  target->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "HTTPParser"),
              t->GetFunction(env->context()).ToLocalChecked()).Check();
}