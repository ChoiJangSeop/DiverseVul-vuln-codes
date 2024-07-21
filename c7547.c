const char *get_icalcomponent_errstr(icalcomponent *ical)
{
    icalcomponent *comp;

    for (comp = icalcomponent_get_first_component(ical, ICAL_ANY_COMPONENT);
	 comp;
	 comp = icalcomponent_get_next_component(ical, ICAL_ANY_COMPONENT)) {
	icalproperty *prop;

	for (prop = icalcomponent_get_first_property(comp, ICAL_ANY_PROPERTY);
	     prop;
	     prop = icalcomponent_get_next_property(comp, ICAL_ANY_PROPERTY)) {

	    if (icalproperty_isa(prop) == ICAL_XLICERROR_PROPERTY) {
		icalparameter *param;
		const char *errstr = icalproperty_get_xlicerror(prop);

		if (!errstr) return "Unknown iCal parsing error";

		param = icalproperty_get_first_parameter(
		    prop, ICAL_XLICERRORTYPE_PARAMETER);

		if (icalparameter_get_xlicerrortype(param) ==
		    ICAL_XLICERRORTYPE_VALUEPARSEERROR) {
		    /* Check if this is an empty property error */
		    char propname[256];
		    if (sscanf(errstr,
			       "No value for %s property", propname) == 1) {
			/* Empty LOCATION is OK */
			if (!strcasecmp(propname, "LOCATION")) continue;
			if (!strcasecmp(propname, "COMMENT")) continue;
			if (!strcasecmp(propname, "DESCRIPTION")) continue;
		    }
		}
	    }
	}
    }

    return NULL;
}