Value ExpressionArrayToObject::evaluate(const Document& root, Variables* variables) const {
    const Value input = _children[0]->evaluate(root, variables);
    if (input.nullish()) {
        return Value(BSONNULL);
    }

    uassert(40386,
            str::stream() << "$arrayToObject requires an array input, found: "
                          << typeName(input.getType()),
            input.isArray());

    MutableDocument output;
    const vector<Value>& array = input.getArray();
    if (array.empty()) {
        return output.freezeToValue();
    }

    // There are two accepted input formats in an array: [ [key, val] ] or [ {k:key, v:val} ]. The
    // first array element determines the format for the rest of the array. Mixing input formats is
    // not allowed.
    bool inputArrayFormat;
    if (array[0].isArray()) {
        inputArrayFormat = true;
    } else if (array[0].getType() == BSONType::Object) {
        inputArrayFormat = false;
    } else {
        uasserted(40398,
                  str::stream() << "Unrecognised input type format for $arrayToObject: "
                                << typeName(array[0].getType()));
    }

    for (auto&& elem : array) {
        if (inputArrayFormat == true) {
            uassert(
                40396,
                str::stream() << "$arrayToObject requires a consistent input format. Elements must"
                                 "all be arrays or all be objects. Array was detected, now found: "
                              << typeName(elem.getType()),
                elem.isArray());

            const vector<Value>& valArray = elem.getArray();

            uassert(40397,
                    str::stream() << "$arrayToObject requires an array of size 2 arrays,"
                                     "found array of size: "
                                  << valArray.size(),
                    (valArray.size() == 2));

            uassert(40395,
                    str::stream() << "$arrayToObject requires an array of key-value pairs, where "
                                     "the key must be of type string. Found key type: "
                                  << typeName(valArray[0].getType()),
                    (valArray[0].getType() == BSONType::String));

            output[valArray[0].getString()] = valArray[1];

        } else {
            uassert(
                40391,
                str::stream() << "$arrayToObject requires a consistent input format. Elements must"
                                 "all be arrays or all be objects. Object was detected, now found: "
                              << typeName(elem.getType()),
                (elem.getType() == BSONType::Object));

            uassert(40392,
                    str::stream() << "$arrayToObject requires an object keys of 'k' and 'v'. "
                                     "Found incorrect number of keys:"
                                  << elem.getDocument().computeSize(),
                    (elem.getDocument().computeSize() == 2));

            Value key = elem.getDocument().getField("k");
            Value value = elem.getDocument().getField("v");

            uassert(40393,
                    str::stream() << "$arrayToObject requires an object with keys 'k' and 'v'. "
                                     "Missing either or both keys from: "
                                  << elem.toString(),
                    (!key.missing() && !value.missing()));

            uassert(
                40394,
                str::stream() << "$arrayToObject requires an object with keys 'k' and 'v', where "
                                 "the value of 'k' must be of type string. Found type: "
                              << typeName(key.getType()),
                (key.getType() == BSONType::String));

            output[key.getString()] = value;
        }
    }

    return output.freezeToValue();
}