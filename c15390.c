int addNodes(std::string link, std::vector<Proxy> &allNodes, int groupID, parse_settings &parse_set)
{
    std::string &proxy = *parse_set.proxy, &subInfo = *parse_set.sub_info;
    string_array &exclude_remarks = *parse_set.exclude_remarks;
    string_array &include_remarks = *parse_set.include_remarks;
    RegexMatchConfigs &stream_rules = *parse_set.stream_rules;
    RegexMatchConfigs &time_rules = *parse_set.time_rules;
    string_icase_map *request_headers = parse_set.request_header;
    bool &authorized = parse_set.authorized;

    ConfType linkType = ConfType::Unknow;
    std::vector<Proxy> nodes;
    Proxy node;
    std::string strSub, extra_headers, custom_group;

    // TODO: replace with startsWith if appropriate
    link = replaceAllDistinct(link, "\"", "");

    /// script:filepath,arg1,arg2,...
    script_safe_runner(parse_set.js_runtime, parse_set.js_context, [&](qjs::Context &ctx)
    {
        if(startsWith(link, "script:")) /// process subscription with script
        {
            writeLog(0, "Found script link. Start running...", LOG_LEVEL_INFO);
            string_array args = split(link.substr(7), ",");
            if(args.size() >= 1)
            {
                std::string script = fileGet(args[0], false);
                try
                {
                    ctx.eval(script);
                    args.erase(args.begin()); /// remove script path
                    auto parse = (std::function<std::string(const std::string&, const string_array&)>) ctx.eval("parse");
                    switch(args.size())
                    {
                    case 0:
                        link = parse(std::string(), string_array());
                        break;
                    case 1:
                        link = parse(args[0], string_array());
                        break;
                    default:
                        {
                            std::string first = args[0];
                            args.erase(args.begin());
                            link = parse(first, args);
                            break;
                        }
                    }
                }
                catch(qjs::exception)
                {
                    script_print_stack(ctx);
                }
            }
        }
    }, global.scriptCleanContext);
            /*
            duk_context *ctx = duktape_init();
            defer(duk_destroy_heap(ctx);)
            duktape_peval(ctx, script);
            duk_get_global_string(ctx, "parse");
            for(size_t i = 1; i < args.size(); i++)
                duk_push_string(ctx, trim(args[i]).c_str());
            if(duk_pcall(ctx, args.size() - 1) == 0)
                link = duktape_get_res_str(ctx);
            else
            {
                writeLog(0, "Error when trying to evaluate script:\n" + duktape_get_err_stack(ctx), LOG_LEVEL_ERROR);
                duk_pop(ctx); /// pop err
            }
            */

    /// tag:group_name,link
    if(startsWith(link, "tag:"))
    {
        string_size pos = link.find(",");
        if(pos != link.npos)
        {
            custom_group = link.substr(4, pos - 4);
            link.erase(0, pos + 1);
        }
    }

    if(link == "nullnode")
    {
        node.GroupId = 0;
        writeLog(0, "Adding node placeholder...");
        allNodes.emplace_back(std::move(node));
        return 0;
    }

    writeLog(LOG_TYPE_INFO, "Received Link.");
    if(startsWith(link, "https://t.me/socks") || startsWith(link, "tg://socks"))
        linkType = ConfType::SOCKS;
    else if(startsWith(link, "https://t.me/http") || startsWith(link, "tg://http"))
        linkType = ConfType::HTTP;
    else if(isLink(link) || startsWith(link, "surge:///install-config"))
        linkType = ConfType::SUB;
    else if(startsWith(link, "Netch://"))
        linkType = ConfType::Netch;
    else if(fileExist(link))
        linkType = ConfType::Local;

    switch(linkType)
    {
    case ConfType::SUB:
        writeLog(LOG_TYPE_INFO, "Downloading subscription data...");
        if(startsWith(link, "surge:///install-config")) //surge config link
            link = urlDecode(getUrlArg(link, "url"));
        strSub = webGet(link, proxy, global.cacheSubscription, &extra_headers, request_headers);
        /*
        if(strSub.size() == 0)
        {
            //try to get it again with system proxy
            writeLog(LOG_TYPE_WARN, "Cannot download subscription directly. Using system proxy.");
            strProxy = getSystemProxy();
            if(strProxy != "")
            {
                strSub = webGet(link, strProxy);
            }
            else
                writeLog(LOG_TYPE_WARN, "No system proxy is set. Skipping.");
        }
        */
        if(strSub.size())
        {
            writeLog(LOG_TYPE_INFO, "Parsing subscription data...");
            if(explodeConfContent(strSub, nodes) == 0)
            {
                writeLog(LOG_TYPE_ERROR, "Invalid subscription: '" + link + "'!");
                return -1;
            }
            if(startsWith(strSub, "ssd://"))
            {
                getSubInfoFromSSD(strSub, subInfo);
            }
            else
            {
                if(!getSubInfoFromHeader(extra_headers, subInfo))
                    getSubInfoFromNodes(nodes, stream_rules, time_rules, subInfo);
            }
            filterNodes(nodes, exclude_remarks, include_remarks, groupID);
            for(Proxy &x : nodes)
            {
                x.GroupId = groupID;
                if(custom_group.size())
                    x.Group = custom_group;
            }
            copyNodes(nodes, allNodes);
        }
        else
        {
            writeLog(LOG_TYPE_ERROR, "Cannot download subscription data.");
            return -1;
        }
        break;
    case ConfType::Local:
        if(!authorized)
            return -1;
        writeLog(LOG_TYPE_INFO, "Parsing configuration file data...");
        if(explodeConf(link, nodes) == 0)
        {
            writeLog(LOG_TYPE_ERROR, "Invalid configuration file!");
            return -1;
        }
        if(startsWith(strSub, "ssd://"))
        {
            getSubInfoFromSSD(strSub, subInfo);
        }
        else
        {
            getSubInfoFromNodes(nodes, stream_rules, time_rules, subInfo);
        }
        filterNodes(nodes, exclude_remarks, include_remarks, groupID);
        for(Proxy &x : nodes)
        {
            x.GroupId = groupID;
            if(custom_group.size())
                x.Group = custom_group;
        }
        copyNodes(nodes, allNodes);
        break;
    default:
        explode(link, node);
        if(node.Type == -1)
        {
            writeLog(LOG_TYPE_ERROR, "No valid link found.");
            return -1;
        }
        node.GroupId = groupID;
        if(custom_group.size())
            node.Group = custom_group;
        allNodes.emplace_back(std::move(node));
    }
    return 0;
}