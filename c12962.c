NO_INLINE JsVar *jspeFactorDelete() {
  JSP_ASSERT_MATCH(LEX_R_DELETE);
  JsVar *parent = 0;
  JsVar *a = jspeFactorMember(jspeFactor(), &parent);
  JsVar *result = 0;
  if (JSP_SHOULD_EXECUTE) {
    bool ok = false;
    if (jsvIsName(a) && !jsvIsNewChild(a)) {
      // if no parent, check in root?
      if (!parent && jsvIsChild(execInfo.root, a))
        parent = jsvLockAgain(execInfo.root);

      if (jsvHasChildren(parent)) {
        // else remove properly.
        if (jsvIsArray(parent)) {
          // For arrays, we must make sure we don't change the length
          JsVarInt l = jsvGetArrayLength(parent);
          jsvRemoveChild(parent, a);
          jsvSetArrayLength(parent, l, false);
        } else {
          jsvRemoveChild(parent, a);
        }
        ok = true;
      }
    }

    result = jsvNewFromBool(ok);
  }
  jsvUnLock2(a, parent);
  return result;
}