static char *mongo_data_append( char *start , const void *data , int len ) {
    memcpy( start , data , len );
    return start + len;
}