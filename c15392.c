void readYAMLConf(YAML::Node &node)
{
    YAML::Node section = node["common"];
    std::string strLine;
    string_array tempArray;

    section["api_mode"] >> global.APIMode;
    section["api_access_token"] >> global.accessToken;
    if(section["default_url"].IsSequence())
    {
        section["default_url"] >> tempArray;
        if(tempArray.size())
        {
            strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
            {
                return std::move(a) + "|" + std::move(b);
            });
            global.defaultUrls = strLine;
            eraseElements(tempArray);
        }
    }
    global.enableInsert = safe_as<std::string>(section["enable_insert"]);
    if(section["insert_url"].IsSequence())
    {
        section["insert_url"] >> tempArray;
        if(tempArray.size())
        {
            strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
            {
                return std::move(a) + "|" + std::move(b);
            });
            global.insertUrls = strLine;
            eraseElements(tempArray);
        }
    }
    section["prepend_insert_url"] >> global.prependInsert;
    if(section["exclude_remarks"].IsSequence())
        section["exclude_remarks"] >> global.excludeRemarks;
    if(section["include_remarks"].IsSequence())
        section["include_remarks"] >> global.includeRemarks;
    global.filterScript = safe_as<bool>(section["enable_filter"]) ? safe_as<std::string>(section["filter_script"]) : "";
    section["base_path"] >> global.basePath;
    section["clash_rule_base"] >> global.clashBase;
    section["surge_rule_base"] >> global.surgeBase;
    section["surfboard_rule_base"] >> global.surfboardBase;
    section["mellow_rule_base"] >> global.mellowBase;
    section["quan_rule_base"] >> global.quanBase;
    section["quanx_rule_base"] >> global.quanXBase;
    section["loon_rule_base"] >> global.loonBase;
    section["sssub_rule_base"] >> global.SSSubBase;

    section["default_external_config"] >> global.defaultExtConfig;
    section["append_proxy_type"] >> global.appendType;
    section["proxy_config"] >> global.proxyConfig;
    section["proxy_ruleset"] >> global.proxyRuleset;
    section["proxy_subscription"] >> global.proxySubscription;

    if(node["userinfo"].IsDefined())
    {
        section = node["userinfo"];
        if(section["stream_rule"].IsSequence())
        {
            readRegexMatch(section["stream_rule"], "|", tempArray, false);
            auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, "|");
            safe_set_streams(configs);
            eraseElements(tempArray);
        }
        if(section["time_rule"].IsSequence())
        {
            readRegexMatch(section["time_rule"], "|", tempArray, false);
            auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, "|");
            safe_set_times(configs);
            eraseElements(tempArray);
        }
    }

    if(node["node_pref"].IsDefined())
    {
        section = node["node_pref"];
        /*
        section["udp_flag"] >> udp_flag;
        section["tcp_fast_open_flag"] >> tfo_flag;
        section["skip_cert_verify_flag"] >> scv_flag;
        */
        global.UDPFlag.set(safe_as<std::string>(section["udp_flag"]));
        global.TFOFlag.set(safe_as<std::string>(section["tcp_fast_open_flag"]));
        global.skipCertVerify.set(safe_as<std::string>(section["skip_cert_verify_flag"]));
        global.TLS13Flag.set(safe_as<std::string>(section["tls13_flag"]));
        section["sort_flag"] >> global.enableSort;
        section["sort_script"] >> global.sortScript;
        section["filter_deprecated_nodes"] >> global.filterDeprecated;
        section["append_sub_userinfo"] >> global.appendUserinfo;
        section["clash_use_new_field_name"] >> global.clashUseNewField;
        section["clash_proxies_style"] >> global.clashProxiesStyle;
    }

    if(section["rename_node"].IsSequence())
    {
        readRegexMatch(section["rename_node"], "@", tempArray, false);
        auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, "@");
        safe_set_renames(configs);
        eraseElements(tempArray);
    }

    if(node["managed_config"].IsDefined())
    {
        section = node["managed_config"];
        section["write_managed_config"] >> global.writeManagedConfig;
        section["managed_config_prefix"] >> global.managedConfigPrefix;
        section["config_update_interval"] >> global.updateInterval;
        section["config_update_strict"] >> global.updateStrict;
        section["quanx_device_id"] >> global.quanXDevID;
    }

    if(node["surge_external_proxy"].IsDefined())
    {
        node["surge_external_proxy"]["surge_ssr_path"] >> global.surgeSSRPath;
        node["surge_external_proxy"]["resolve_hostname"] >> global.surgeResolveHostname;
    }

    if(node["emojis"].IsDefined())
    {
        section = node["emojis"];
        section["add_emoji"] >> global.addEmoji;
        section["remove_old_emoji"] >> global.removeEmoji;
        if(section["rules"].IsSequence())
        {
            readEmoji(section["rules"], tempArray, false);
            auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, ",");
            safe_set_emojis(configs);
            eraseElements(tempArray);
        }
    }

    const char *rulesets_title = node["rulesets"].IsDefined() ? "rulesets" : "ruleset";
    if(node[rulesets_title].IsDefined())
    {
        section = node[rulesets_title];
        section["enabled"] >> global.enableRuleGen;
        if(!global.enableRuleGen)
        {
            global.overwriteOriginalRules = false;
            global.updateRulesetOnRequest = false;
        }
        else
        {
            section["overwrite_original_rules"] >> global.overwriteOriginalRules;
            section["update_ruleset_on_request"] >> global.updateRulesetOnRequest;
        }
        const char *ruleset_title = section["rulesets"].IsDefined() ? "rulesets" : "surge_ruleset";
        if(section[ruleset_title].IsSequence())
        {
            string_array vArray;
            readRuleset(section[ruleset_title], vArray, false);
            global.customRulesets = INIBinding::from<RulesetConfig>::from_ini(vArray);
        }
    }

    const char *groups_title = node["proxy_groups"].IsDefined() ? "proxy_groups" : "proxy_group";
    if(node[groups_title].IsDefined() && node[groups_title]["custom_proxy_group"].IsDefined())
    {
        string_array vArray;
        readGroup(node[groups_title]["custom_proxy_group"], vArray, false);
        global.customProxyGroups = INIBinding::from<ProxyGroupConfig>::from_ini(vArray);
    }

    if(node["template"].IsDefined())
    {
        node["template"]["template_path"] >> global.templatePath;
        if(node["template"]["globals"].IsSequence())
        {
            eraseElements(global.templateVars);
            for(size_t i = 0; i < node["template"]["globals"].size(); i++)
            {
                std::string key, value;
                node["template"]["globals"][i]["key"] >> key;
                node["template"]["globals"][i]["value"] >> value;
                global.templateVars[key] = value;
            }
        }
    }

    if(node["aliases"].IsSequence())
    {
        webServer.reset_redirect();
        for(size_t i = 0; i < node["aliases"].size(); i++)
        {
            std::string uri, target;
            node["aliases"][i]["uri"] >> uri;
            node["aliases"][i]["target"] >> target;
            webServer.append_redirect(uri, target);
        }
    }

    if(node["tasks"].IsSequence())
    {
        string_array vArray;
        for(size_t i = 0; i < node["tasks"].size(); i++)
        {
            std::string name, exp, path, timeout;
            node["tasks"][i]["import"] >> name;
            if(name.size())
            {
                vArray.emplace_back("!!import:" + name);
                continue;
            }
            node["tasks"][i]["name"] >> name;
            node["tasks"][i]["cronexp"] >> exp;
            node["tasks"][i]["path"] >> path;
            node["tasks"][i]["timeout"] >> timeout;
            strLine = name + "`" + exp + "`" + path + "`" + timeout;
            vArray.emplace_back(std::move(strLine));
        }
        importItems(vArray, false);
        global.enableCron = !vArray.empty();
        global.cronTasks = INIBinding::from<CronTaskConfig>::from_ini(vArray);
        refresh_schedule();
    }

    if(node["server"].IsDefined())
    {
        node["server"]["listen"] >> global.listenAddress;
        node["server"]["port"] >> global.listenPort;
        node["server"]["serve_file_root"] >>= webServer.serve_file_root;
        webServer.serve_file = !webServer.serve_file_root.empty();
    }

    if(node["advanced"].IsDefined())
    {
        std::string log_level;
        node["advanced"]["log_level"] >> log_level;
        node["advanced"]["print_debug_info"] >> global.printDbgInfo;
        if(global.printDbgInfo)
            global.logLevel = LOG_LEVEL_VERBOSE;
        else
        {
            switch(hash_(log_level))
            {
            case "warn"_hash:
                global.logLevel = LOG_LEVEL_WARNING;
                break;
            case "error"_hash:
                global.logLevel = LOG_LEVEL_ERROR;
                break;
            case "fatal"_hash:
                global.logLevel = LOG_LEVEL_FATAL;
                break;
            case "verbose"_hash:
                global.logLevel = LOG_LEVEL_VERBOSE;
                break;
            case "debug"_hash:
                global.logLevel = LOG_LEVEL_DEBUG;
                break;
            default:
                global.logLevel = LOG_LEVEL_INFO;
            }
        }
        node["advanced"]["max_pending_connections"] >> global.maxPendingConns;
        node["advanced"]["max_concurrent_threads"] >> global.maxConcurThreads;
        node["advanced"]["max_allowed_rulesets"] >> global.maxAllowedRulesets;
        node["advanced"]["max_allowed_rules"] >> global.maxAllowedRules;
        node["advanced"]["max_allowed_download_size"] >> global.maxAllowedDownloadSize;
        if(node["advanced"]["enable_cache"].IsDefined())
        {
            if(safe_as<bool>(node["advanced"]["enable_cache"]))
            {
                node["advanced"]["cache_subscription"] >> global.cacheSubscription;
                node["advanced"]["cache_config"] >> global.cacheConfig;
                node["advanced"]["cache_ruleset"] >> global.cacheRuleset;
                node["advanced"]["serve_cache_on_fetch_fail"] >> global.serveCacheOnFetchFail;
            }
            else
                global.cacheSubscription = global.cacheConfig = global.cacheRuleset = 0; //disable cache
        }
        node["advanced"]["script_clean_context"] >> global.scriptCleanContext;
        node["advanced"]["async_fetch_ruleset"] >> global.asyncFetchRuleset;
        node["advanced"]["skip_failed_links"] >> global.skipFailedLinks;
    }
}