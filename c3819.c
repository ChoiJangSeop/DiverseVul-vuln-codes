xmlRelaxNGGetErrorString(xmlRelaxNGValidErr err, const xmlChar * arg1,
                         const xmlChar * arg2)
{
    char msg[1000];

    if (arg1 == NULL)
        arg1 = BAD_CAST "";
    if (arg2 == NULL)
        arg2 = BAD_CAST "";

    msg[0] = 0;
    switch (err) {
        case XML_RELAXNG_OK:
            return (NULL);
        case XML_RELAXNG_ERR_MEMORY:
            return (xmlCharStrdup("out of memory\n"));
        case XML_RELAXNG_ERR_TYPE:
            snprintf(msg, 1000, "failed to validate type %s\n", arg1);
            break;
        case XML_RELAXNG_ERR_TYPEVAL:
            snprintf(msg, 1000, "Type %s doesn't allow value '%s'\n", arg1,
                     arg2);
            break;
        case XML_RELAXNG_ERR_DUPID:
            snprintf(msg, 1000, "ID %s redefined\n", arg1);
            break;
        case XML_RELAXNG_ERR_TYPECMP:
            snprintf(msg, 1000, "failed to compare type %s\n", arg1);
            break;
        case XML_RELAXNG_ERR_NOSTATE:
            return (xmlCharStrdup("Internal error: no state\n"));
        case XML_RELAXNG_ERR_NODEFINE:
            return (xmlCharStrdup("Internal error: no define\n"));
        case XML_RELAXNG_ERR_INTERNAL:
            snprintf(msg, 1000, "Internal error: %s\n", arg1);
            break;
        case XML_RELAXNG_ERR_LISTEXTRA:
            snprintf(msg, 1000, "Extra data in list: %s\n", arg1);
            break;
        case XML_RELAXNG_ERR_INTERNODATA:
            return (xmlCharStrdup
                    ("Internal: interleave block has no data\n"));
        case XML_RELAXNG_ERR_INTERSEQ:
            return (xmlCharStrdup("Invalid sequence in interleave\n"));
        case XML_RELAXNG_ERR_INTEREXTRA:
            snprintf(msg, 1000, "Extra element %s in interleave\n", arg1);
            break;
        case XML_RELAXNG_ERR_ELEMNAME:
            snprintf(msg, 1000, "Expecting element %s, got %s\n", arg1,
                     arg2);
            break;
        case XML_RELAXNG_ERR_ELEMNONS:
            snprintf(msg, 1000, "Expecting a namespace for element %s\n",
                     arg1);
            break;
        case XML_RELAXNG_ERR_ELEMWRONGNS:
            snprintf(msg, 1000,
                     "Element %s has wrong namespace: expecting %s\n", arg1,
                     arg2);
            break;
        case XML_RELAXNG_ERR_ELEMWRONG:
            snprintf(msg, 1000, "Did not expect element %s there\n", arg1);
            break;
        case XML_RELAXNG_ERR_TEXTWRONG:
            snprintf(msg, 1000,
                     "Did not expect text in element %s content\n", arg1);
            break;
        case XML_RELAXNG_ERR_ELEMEXTRANS:
            snprintf(msg, 1000, "Expecting no namespace for element %s\n",
                     arg1);
            break;
        case XML_RELAXNG_ERR_ELEMNOTEMPTY:
            snprintf(msg, 1000, "Expecting element %s to be empty\n", arg1);
            break;
        case XML_RELAXNG_ERR_NOELEM:
            snprintf(msg, 1000, "Expecting an element %s, got nothing\n",
                     arg1);
            break;
        case XML_RELAXNG_ERR_NOTELEM:
            return (xmlCharStrdup("Expecting an element got text\n"));
        case XML_RELAXNG_ERR_ATTRVALID:
            snprintf(msg, 1000, "Element %s failed to validate attributes\n",
                     arg1);
            break;
        case XML_RELAXNG_ERR_CONTENTVALID:
            snprintf(msg, 1000, "Element %s failed to validate content\n",
                     arg1);
            break;
        case XML_RELAXNG_ERR_EXTRACONTENT:
            snprintf(msg, 1000, "Element %s has extra content: %s\n",
                     arg1, arg2);
            break;
        case XML_RELAXNG_ERR_INVALIDATTR:
            snprintf(msg, 1000, "Invalid attribute %s for element %s\n",
                     arg1, arg2);
            break;
        case XML_RELAXNG_ERR_LACKDATA:
            snprintf(msg, 1000, "Datatype element %s contains no data\n",
                     arg1);
            break;
        case XML_RELAXNG_ERR_DATAELEM:
            snprintf(msg, 1000, "Datatype element %s has child elements\n",
                     arg1);
            break;
        case XML_RELAXNG_ERR_VALELEM:
            snprintf(msg, 1000, "Value element %s has child elements\n",
                     arg1);
            break;
        case XML_RELAXNG_ERR_LISTELEM:
            snprintf(msg, 1000, "List element %s has child elements\n",
                     arg1);
            break;
        case XML_RELAXNG_ERR_DATATYPE:
            snprintf(msg, 1000, "Error validating datatype %s\n", arg1);
            break;
        case XML_RELAXNG_ERR_VALUE:
            snprintf(msg, 1000, "Error validating value %s\n", arg1);
            break;
        case XML_RELAXNG_ERR_LIST:
            return (xmlCharStrdup("Error validating list\n"));
        case XML_RELAXNG_ERR_NOGRAMMAR:
            return (xmlCharStrdup("No top grammar defined\n"));
        case XML_RELAXNG_ERR_EXTRADATA:
            return (xmlCharStrdup("Extra data in the document\n"));
        default:
            return (xmlCharStrdup("Unknown error !\n"));
    }
    if (msg[0] == 0) {
        snprintf(msg, 1000, "Unknown error code %d\n", err);
    }
    msg[1000 - 1] = 0;
    return (xmlStrdup((xmlChar *) msg));
}