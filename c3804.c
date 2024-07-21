int main(int argc ATTRIBUTE_UNUSED, char **argv ATTRIBUTE_UNUSED) {
    xmlChar filename[PATH_MAX];
    xmlModulePtr module = NULL;
    hello_world_t hello_world = NULL;

    /* build the module filename, and confirm the module exists */
    xmlStrPrintf(filename, sizeof(filename),
                 (const xmlChar*) "%s/testdso%s",
                 (const xmlChar*)MODULE_PATH,
		 (const xmlChar*)LIBXML_MODULE_EXTENSION);

    module = xmlModuleOpen((const char*)filename, 0);
    if (module)
      {
        if (xmlModuleSymbol(module, "hello_world", (void **) &hello_world)) {
	    fprintf(stderr, "Failure to lookup\n");
	    return(1);
	}
	if (hello_world == NULL) {
	    fprintf(stderr, "Lookup returned NULL\n");
	    return(1);
	}

        (*hello_world)();

        xmlModuleClose(module);
      }

    xmlMemoryDump();

    return(0);
}