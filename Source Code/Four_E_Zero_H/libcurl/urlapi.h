#ifndef CURLINC_URLAPI_H
#define CURLINC_URLAPI_H

#include "curl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    
    typedef enum
    {
        CURLUE_OK,
        CURLUE_BAD_HANDLE,         
        CURLUE_BAD_PARTPOINTER,    
        CURLUE_MALFORMED_INPUT,    
        CURLUE_BAD_PORT_NUMBER,    
        CURLUE_UNSUPPORTED_SCHEME, 
        CURLUE_URLDECODE,          
        CURLUE_OUT_OF_MEMORY,      
        CURLUE_USER_NOT_ALLOWED,   
        CURLUE_UNKNOWN_PART,       
        CURLUE_NO_SCHEME,          
        CURLUE_NO_USER,            
        CURLUE_NO_PASSWORD,        
        CURLUE_NO_OPTIONS,         
        CURLUE_NO_HOST,            
        CURLUE_NO_PORT,            
        CURLUE_NO_QUERY,           
        CURLUE_NO_FRAGMENT,        
        CURLUE_NO_ZONEID,          
        CURLUE_BAD_FILE_URL,       
        CURLUE_BAD_FRAGMENT,       
        CURLUE_BAD_HOSTNAME,       
        CURLUE_BAD_IPV6,           
        CURLUE_BAD_LOGIN,          
        CURLUE_BAD_PASSWORD,       
        CURLUE_BAD_PATH,           
        CURLUE_BAD_QUERY,          
        CURLUE_BAD_SCHEME,         
        CURLUE_BAD_SLASHES,        
        CURLUE_BAD_USER,           
        CURLUE_LAST
    } CURLUcode;

    typedef enum
    {
        CURLUPART_URL,
        CURLUPART_SCHEME,
        CURLUPART_USER,
        CURLUPART_PASSWORD,
        CURLUPART_OPTIONS,
        CURLUPART_HOST,
        CURLUPART_PORT,
        CURLUPART_PATH,
        CURLUPART_QUERY,
        CURLUPART_FRAGMENT,
        CURLUPART_ZONEID 
    } CURLUPart;

#define CURLU_DEFAULT_PORT (1 << 0) 
#define CURLU_NO_DEFAULT_PORT                                                                                                                                                                                         \
    (1 << 1) 

#define CURLU_DEFAULT_SCHEME                                                                                                                                                                                          \
    (1 << 2)                              

#define CURLU_NON_SUPPORT_SCHEME (1 << 3) 
#define CURLU_PATH_AS_IS (1 << 4)         
#define CURLU_DISALLOW_USER (1 << 5)      
#define CURLU_URLDECODE (1 << 6)          
#define CURLU_URLENCODE (1 << 7)          
#define CURLU_APPENDQUERY (1 << 8)        
#define CURLU_GUESS_SCHEME (1 << 9)       
#define CURLU_NO_AUTHORITY                                                                                                                                                                                            \
    (1 << 10)                       

#define CURLU_ALLOW_SPACE (1 << 11) 

    typedef struct Curl_URL CURLU;

    

    CURL_EXTERN CURLU* curl_url(void);

    

    CURL_EXTERN void curl_url_cleanup(CURLU* handle);

    

    CURL_EXTERN CURLU* curl_url_dup(CURLU* in);

    

    CURL_EXTERN CURLUcode curl_url_get(CURLU* handle, CURLUPart what, char** part, unsigned int flags);

    

    CURL_EXTERN CURLUcode curl_url_set(CURLU* handle, CURLUPart what, const char* part, unsigned int flags);

    

    CURL_EXTERN const char* curl_url_strerror(CURLUcode);

#ifdef __cplusplus
} 
#endif

#endif 
