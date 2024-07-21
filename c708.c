httpSpecialDoSideFinish(AtomPtr data, HTTPRequestPtr requestor)
{
    ObjectPtr object = requestor->object;

    if(matchUrl("/polipo/config", object)) {
        AtomListPtr list = NULL;
        int i, rc;

        if(disableConfiguration) {
            abortObject(object, 403, internAtom("Action not allowed"));
            goto out;
        }

        list = urlDecode(data->string, data->length);
        if(list == NULL) {
            abortObject(object, 400,
                        internAtom("Couldn't parse variable to set"));
            goto out;
        }
        for(i = 0; i < list->length; i++) {
            rc = parseConfigLine(list->list[i]->string, NULL, 0, 1);
            if(rc < 0) {
                abortObject(object, 400,
                            rc == -1 ?
                            internAtom("Couldn't parse variable to set") :
                            internAtom("Variable is not settable"));
                destroyAtomList(list);
                goto out;
            }
        }
        destroyAtomList(list);
        object->date = current_time.tv_sec;
        object->age = current_time.tv_sec;
        object->headers = internAtom("\r\nLocation: /polipo/config?");
        object->code = 303;
        object->message = internAtom("Done");
        object->flags &= ~OBJECT_INITIAL;
        object->length = 0;
    } else if(matchUrl("/polipo/status", object)) {
        AtomListPtr list = NULL;
        int i;

        if(disableConfiguration) {
            abortObject(object, 403, internAtom("Action not allowed"));
            goto out;
        }

        list = urlDecode(data->string, data->length);
        if(list == NULL) {
            abortObject(object, 400,
                        internAtom("Couldn't parse action"));
            goto out;
        }
        for(i = 0; i < list->length; i++) {
            char *equals = 
                memchr(list->list[i]->string, '=', list->list[i]->length);
            AtomPtr name = 
                equals ? 
                internAtomN(list->list[i]->string, 
                            equals - list->list[i]->string) :
                retainAtom(list->list[i]);
            if(name == atomInitForbidden)
                initForbidden();
            else if(name == atomReopenLog)
                reopenLog();
            else if(name == atomDiscardObjects)
                discardObjects(1, 0);
            else if(name == atomWriteoutObjects)
                writeoutObjects(1);
            else if(name == atomFreeChunkArenas)
                free_chunk_arenas();
            else {
                abortObject(object, 400, internAtomF("Unknown action %s",
                                                     name->string));
                releaseAtom(name);
                destroyAtomList(list);
                goto out;
            }
            releaseAtom(name);
        }
        destroyAtomList(list);
        object->date = current_time.tv_sec;
        object->age = current_time.tv_sec;
        object->headers = internAtom("\r\nLocation: /polipo/status?");
        object->code = 303;
        object->message = internAtom("Done");
        object->flags &= ~OBJECT_INITIAL;
        object->length = 0;
    } else {
        abortObject(object, 405, internAtom("Method not allowed"));
    }

 out:
    releaseAtom(data);
    notifyObject(object);
    requestor->connection->flags &= ~CONN_READER;
    return 1;
}