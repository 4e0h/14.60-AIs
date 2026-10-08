#ifndef CURLINC_CURL_H
#define CURLINC_CURL_H

#define CURL_STATICLIB

#ifdef CURL_NO_OLDIES
#define CURL_STRICTER
#endif

#include "curlver.h" 
#include "system.h"  

#if (defined(_WIN32) || defined(__WIN32__) || defined(WIN32)) && !defined(__SYMBIAN32__)
#define CURL_WIN32
#endif

#include <stdio.h>
#include <limits.h>

#if (defined(__FreeBSD__) && (__FreeBSD__ >= 2)) || defined(__MidnightBSD__)

#include <osreldate.h>
#endif

#include <sys/types.h>
#include <time.h>

#if defined(CURL_WIN32) && !defined(_WIN32_WCE) && !defined(__CYGWIN__)
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H) || defined(__LWIP_OPT_H__) || defined(LWIP_HDR_OPT_H))

#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#endif

#if defined(_AIX) || defined(__NOVELL_LIBC__) || defined(__NetBSD__) || defined(__minix) || defined(__SYMBIAN32__) || defined(__INTEGRITY) || defined(ANDROID) || defined(__ANDROID__) || defined(__OpenBSD__) ||     \
    defined(__CYGWIN__) || defined(AMIGA) || defined(__NuttX__) || (defined(__FreeBSD_version) && (__FreeBSD_version < 800000)) || (defined(__MidnightBSD_version) && (__MidnightBSD_version < 100000))
#include <sys/select.h>
#endif

#if !defined(CURL_WIN32) && !defined(_WIN32_WCE)
#include <sys/socket.h>
#endif

#if !defined(CURL_WIN32)
#include <sys/time.h>
#endif

#ifndef __has_declspec_attribute
#define __has_declspec_attribute(x) 0
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(BUILDING_LIBCURL) || defined(CURL_STRICTER)
    typedef struct Curl_easy CURL;
    typedef struct Curl_share CURLSH;
#else
typedef void CURL;
typedef void CURLSH;
#endif

    

#ifdef CURL_STATICLIB
#define CURL_EXTERN
#elif defined(CURL_WIN32) || defined(__SYMBIAN32__) || (__has_declspec_attribute(dllexport) && __has_declspec_attribute(dllimport))
#if defined(BUILDING_LIBCURL)
#define CURL_EXTERN __declspec(dllexport)
#else
#define CURL_EXTERN __declspec(dllimport)
#endif
#elif defined(BUILDING_LIBCURL) && defined(CURL_HIDDEN_SYMBOLS)
#define CURL_EXTERN CURL_EXTERN_SYMBOL
#else
#define CURL_EXTERN
#endif

#ifndef curl_socket_typedef

#if defined(CURL_WIN32) && !defined(__LWIP_OPT_H__) && !defined(LWIP_HDR_OPT_H)
    typedef SOCKET curl_socket_t;
#define CURL_SOCKET_BAD INVALID_SOCKET
#else
    typedef int curl_socket_t;
#define CURL_SOCKET_BAD -1
#endif
#define curl_socket_typedef
#endif 

    
    typedef enum
    {
        CURLSSLBACKEND_NONE = 0,
        CURLSSLBACKEND_OPENSSL = 1,
        CURLSSLBACKEND_GNUTLS = 2,
        CURLSSLBACKEND_NSS = 3,
        CURLSSLBACKEND_OBSOLETE4 = 4, 
        CURLSSLBACKEND_GSKIT = 5,
        CURLSSLBACKEND_POLARSSL = 6,
        CURLSSLBACKEND_WOLFSSL = 7,
        CURLSSLBACKEND_SCHANNEL = 8,
        CURLSSLBACKEND_SECURETRANSPORT = 9,
        CURLSSLBACKEND_AXTLS = 10, 
        CURLSSLBACKEND_MBEDTLS = 11,
        CURLSSLBACKEND_MESALINK = 12,
        CURLSSLBACKEND_BEARSSL = 13,
        CURLSSLBACKEND_RUSTLS = 14
    } curl_sslbackend;

#define CURLSSLBACKEND_LIBRESSL CURLSSLBACKEND_OPENSSL
#define CURLSSLBACKEND_BORINGSSL CURLSSLBACKEND_OPENSSL

#define CURLSSLBACKEND_CYASSL CURLSSLBACKEND_WOLFSSL
#define CURLSSLBACKEND_DARWINSSL CURLSSLBACKEND_SECURETRANSPORT

    struct curl_httppost
    {
        struct curl_httppost* next;       
        char* name;                       
        long namelength;                  
        char* contents;                   
        long contentslength;              

        char* buffer;                     
        long bufferlength;                
        char* contenttype;                
        struct curl_slist* contentheader; 
        struct curl_httppost* more;       

        long flags;                       

#define CURL_HTTPPOST_FILENAME (1 << 0)

#define CURL_HTTPPOST_READFILE (1 << 1)

#define CURL_HTTPPOST_PTRNAME (1 << 2)

#define CURL_HTTPPOST_PTRCONTENTS (1 << 3)

#define CURL_HTTPPOST_BUFFER (1 << 4)

#define CURL_HTTPPOST_PTRBUFFER (1 << 5)

#define CURL_HTTPPOST_CALLBACK (1 << 6)

#define CURL_HTTPPOST_LARGE (1 << 7)

        char* showfilename;    

        void* userp;           

        curl_off_t contentlen; 

    };

#define CURL_PROGRESSFUNC_CONTINUE 0x10000001

    

    typedef int (*curl_progress_callback)(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);

    

    typedef int (*curl_xferinfo_callback)(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

#ifndef CURL_MAX_READ_SIZE
    
#define CURL_MAX_READ_SIZE 524288
#endif

#ifndef CURL_MAX_WRITE_SIZE
    

#define CURL_MAX_WRITE_SIZE 16384
#endif

#ifndef CURL_MAX_HTTP_HEADER

#define CURL_MAX_HTTP_HEADER (100 * 1024)
#endif

#define CURL_WRITEFUNC_PAUSE 0x10000001

    typedef size_t (*curl_write_callback)(char* buffer, size_t size, size_t nitems, void* outstream);

    
    typedef int (*curl_resolver_start_callback)(void* resolver_state, void* reserved, void* userdata);

    
    typedef enum
    {
        CURLFILETYPE_FILE = 0,
        CURLFILETYPE_DIRECTORY,
        CURLFILETYPE_SYMLINK,
        CURLFILETYPE_DEVICE_BLOCK,
        CURLFILETYPE_DEVICE_CHAR,
        CURLFILETYPE_NAMEDPIPE,
        CURLFILETYPE_SOCKET,
        CURLFILETYPE_DOOR, 

        CURLFILETYPE_UNKNOWN 
    } curlfiletype;

#define CURLFINFOFLAG_KNOWN_FILENAME (1 << 0)
#define CURLFINFOFLAG_KNOWN_FILETYPE (1 << 1)
#define CURLFINFOFLAG_KNOWN_TIME (1 << 2)
#define CURLFINFOFLAG_KNOWN_PERM (1 << 3)
#define CURLFINFOFLAG_KNOWN_UID (1 << 4)
#define CURLFINFOFLAG_KNOWN_GID (1 << 5)
#define CURLFINFOFLAG_KNOWN_SIZE (1 << 6)
#define CURLFINFOFLAG_KNOWN_HLINKCOUNT (1 << 7)

    
    struct curl_fileinfo
    {
        char* filename;
        curlfiletype filetype;
        time_t time; 
        unsigned int perm;
        int uid;
        int gid;
        curl_off_t size;
        long int hardlinks;

        struct
        {
            
            char* time;
            char* perm;
            char* user;
            char* group;
            char* target; 
        } strings;

        unsigned int flags;

        
        char* b_data;
        size_t b_size;
        size_t b_used;
    };

#define CURL_CHUNK_BGN_FUNC_OK 0
#define CURL_CHUNK_BGN_FUNC_FAIL 1 
#define CURL_CHUNK_BGN_FUNC_SKIP 2 

    

    typedef long (*curl_chunk_bgn_callback)(const void* transfer_info, void* ptr, int remains);

#define CURL_CHUNK_END_FUNC_OK 0
#define CURL_CHUNK_END_FUNC_FAIL 1 

    

    typedef long (*curl_chunk_end_callback)(void* ptr);

#define CURL_FNMATCHFUNC_MATCH 0   
#define CURL_FNMATCHFUNC_NOMATCH 1 
#define CURL_FNMATCHFUNC_FAIL 2    

    

    typedef int (*curl_fnmatch_callback)(void* ptr, const char* pattern, const char* string);

#define CURL_SEEKFUNC_OK 0
#define CURL_SEEKFUNC_FAIL 1 
#define CURL_SEEKFUNC_CANTSEEK                                                                                                                                                                                        \
    2                                                                                 

    typedef int (*curl_seek_callback)(void* instream, curl_off_t offset, int origin); 

#define CURL_READFUNC_ABORT 0x10000000

#define CURL_READFUNC_PAUSE 0x10000001

#define CURL_TRAILERFUNC_OK 0

#define CURL_TRAILERFUNC_ABORT 1

    typedef size_t (*curl_read_callback)(char* buffer, size_t size, size_t nitems, void* instream);

    typedef int (*curl_trailer_callback)(struct curl_slist** list, void* userdata);

    typedef enum
    {
        CURLSOCKTYPE_IPCXN,  
        CURLSOCKTYPE_ACCEPT, 
        CURLSOCKTYPE_LAST    
    } curlsocktype;

#define CURL_SOCKOPT_OK 0
#define CURL_SOCKOPT_ERROR                                                                                                                                                                                            \
    1 

#define CURL_SOCKOPT_ALREADY_CONNECTED 2

    typedef int (*curl_sockopt_callback)(void* clientp, curl_socket_t curlfd, curlsocktype purpose);

    struct curl_sockaddr
    {
        int family;
        int socktype;
        int protocol;
        unsigned int addrlen; 

        struct sockaddr addr;
    };

    typedef curl_socket_t (*curl_opensocket_callback)(void* clientp, curlsocktype purpose, struct curl_sockaddr* address);

    typedef int (*curl_closesocket_callback)(void* clientp, curl_socket_t item);

    typedef enum
    {
        CURLIOE_OK,          
        CURLIOE_UNKNOWNCMD,  
        CURLIOE_FAILRESTART, 
        CURLIOE_LAST         
    } curlioerr;

    typedef enum
    {
        CURLIOCMD_NOP,         
        CURLIOCMD_RESTARTREAD, 
        CURLIOCMD_LAST         
    } curliocmd;

    typedef curlioerr (*curl_ioctl_callback)(CURL* handle, int cmd, void* clientp);

#ifndef CURL_DID_MEMORY_FUNC_TYPEDEFS
    

    typedef void* (*curl_malloc_callback)(size_t size);
    typedef void (*curl_free_callback)(void* ptr);
    typedef void* (*curl_realloc_callback)(void* ptr, size_t size);
    typedef char* (*curl_strdup_callback)(const char* str);
    typedef void* (*curl_calloc_callback)(size_t nmemb, size_t size);

#define CURL_DID_MEMORY_FUNC_TYPEDEFS
#endif

    
    typedef enum
    {
        CURLINFO_TEXT = 0,
        CURLINFO_HEADER_IN,    
        CURLINFO_HEADER_OUT,   
        CURLINFO_DATA_IN,      
        CURLINFO_DATA_OUT,     
        CURLINFO_SSL_DATA_IN,  
        CURLINFO_SSL_DATA_OUT, 
        CURLINFO_END
    } curl_infotype;

    typedef int (*curl_debug_callback)(CURL* handle,       
                                       curl_infotype type, 
                                       char* data,         
                                       size_t size,        
                                       void* userptr);     

    
    typedef int (*curl_prereq_callback)(void* clientp, char* conn_primary_ip, char* conn_local_ip, int conn_primary_port, int conn_local_port);

#define CURL_PREREQFUNC_OK 0

#define CURL_PREREQFUNC_ABORT 1

    

    typedef enum
    {
        CURLE_OK = 0,
        CURLE_UNSUPPORTED_PROTOCOL,     
        CURLE_FAILED_INIT,              
        CURLE_URL_MALFORMAT,            
        CURLE_NOT_BUILT_IN,             

        CURLE_COULDNT_RESOLVE_PROXY,    
        CURLE_COULDNT_RESOLVE_HOST,     
        CURLE_COULDNT_CONNECT,          
        CURLE_WEIRD_SERVER_REPLY,       
        CURLE_REMOTE_ACCESS_DENIED,     

        CURLE_FTP_ACCEPT_FAILED,        

        CURLE_FTP_WEIRD_PASS_REPLY,     
        CURLE_FTP_ACCEPT_TIMEOUT,       

        CURLE_FTP_WEIRD_PASV_REPLY,     
        CURLE_FTP_WEIRD_227_FORMAT,     
        CURLE_FTP_CANT_GET_HOST,        
        CURLE_HTTP2,                    

        CURLE_FTP_COULDNT_SET_TYPE,     
        CURLE_PARTIAL_FILE,             
        CURLE_FTP_COULDNT_RETR_FILE,    
        CURLE_OBSOLETE20,               
        CURLE_QUOTE_ERROR,              
        CURLE_HTTP_RETURNED_ERROR,      
        CURLE_WRITE_ERROR,              
        CURLE_OBSOLETE24,               
        CURLE_UPLOAD_FAILED,            
        CURLE_READ_ERROR,               
        CURLE_OUT_OF_MEMORY,            
        CURLE_OPERATION_TIMEDOUT,       
        CURLE_OBSOLETE29,               
        CURLE_FTP_PORT_FAILED,          
        CURLE_FTP_COULDNT_USE_REST,     
        CURLE_OBSOLETE32,               
        CURLE_RANGE_ERROR,              
        CURLE_HTTP_POST_ERROR,          
        CURLE_SSL_CONNECT_ERROR,        
        CURLE_BAD_DOWNLOAD_RESUME,      
        CURLE_FILE_COULDNT_READ_FILE,   
        CURLE_LDAP_CANNOT_BIND,         
        CURLE_LDAP_SEARCH_FAILED,       
        CURLE_OBSOLETE40,               
        CURLE_FUNCTION_NOT_FOUND,       
        CURLE_ABORTED_BY_CALLBACK,      
        CURLE_BAD_FUNCTION_ARGUMENT,    
        CURLE_OBSOLETE44,               
        CURLE_INTERFACE_FAILED,         
        CURLE_OBSOLETE46,               
        CURLE_TOO_MANY_REDIRECTS,       
        CURLE_UNKNOWN_OPTION,           
        CURLE_SETOPT_OPTION_SYNTAX,     
        CURLE_OBSOLETE50,               
        CURLE_OBSOLETE51,               
        CURLE_GOT_NOTHING,              
        CURLE_SSL_ENGINE_NOTFOUND,      
        CURLE_SSL_ENGINE_SETFAILED,     

        CURLE_SEND_ERROR,               
        CURLE_RECV_ERROR,               
        CURLE_OBSOLETE57,               
        CURLE_SSL_CERTPROBLEM,          
        CURLE_SSL_CIPHER,               
        CURLE_PEER_FAILED_VERIFICATION, 

        CURLE_BAD_CONTENT_ENCODING,     
        CURLE_OBSOLETE62,               
        CURLE_FILESIZE_EXCEEDED,        
        CURLE_USE_SSL_FAILED,           
        CURLE_SEND_FAIL_REWIND,         

        CURLE_SSL_ENGINE_INITFAILED,    
        CURLE_LOGIN_DENIED,             

        CURLE_TFTP_NOTFOUND,            
        CURLE_TFTP_PERM,                
        CURLE_REMOTE_DISK_FULL,         
        CURLE_TFTP_ILLEGAL,             
        CURLE_TFTP_UNKNOWNID,           
        CURLE_REMOTE_FILE_EXISTS,       
        CURLE_TFTP_NOSUCHUSER,          
        CURLE_OBSOLETE75,               
        CURLE_OBSOLETE76,               
        CURLE_SSL_CACERT_BADFILE,       

        CURLE_REMOTE_FILE_NOT_FOUND,    
        CURLE_SSH,                      

        CURLE_SSL_SHUTDOWN_FAILED,      

        CURLE_AGAIN,                    

        CURLE_SSL_CRL_BADFILE,          

        CURLE_SSL_ISSUER_ERROR,         

        CURLE_FTP_PRET_FAILED,          
        CURLE_RTSP_CSEQ_ERROR,          
        CURLE_RTSP_SESSION_ERROR,       
        CURLE_FTP_BAD_FILE_LIST,        
        CURLE_CHUNK_FAILED,             
        CURLE_NO_CONNECTION_AVAILABLE,  

        CURLE_SSL_PINNEDPUBKEYNOTMATCH, 

        CURLE_SSL_INVALIDCERTSTATUS,    
        CURLE_HTTP2_STREAM,             

        CURLE_RECURSIVE_API_CALL,       

        CURLE_AUTH_ERROR,               

        CURLE_HTTP3,                    
        CURLE_QUIC_CONNECT_ERROR,       
        CURLE_PROXY,                    
        CURLE_SSL_CLIENTCERT,           
        CURLE_UNRECOVERABLE_POLL,       
        CURL_LAST                       
    } CURLcode;

#ifndef CURL_NO_OLDIES 

#define CURLE_OBSOLETE16 CURLE_HTTP2

#define CURLE_OBSOLETE10 CURLE_FTP_ACCEPT_FAILED
#define CURLE_OBSOLETE12 CURLE_FTP_ACCEPT_TIMEOUT

#define CURLOPT_ENCODING CURLOPT_ACCEPT_ENCODING
#define CURLE_FTP_WEIRD_SERVER_REPLY CURLE_WEIRD_SERVER_REPLY

#define CURLE_SSL_CACERT CURLE_PEER_FAILED_VERIFICATION

#define CURLE_UNKNOWN_TELNET_OPTION CURLE_UNKNOWN_OPTION

#define CURLE_TELNET_OPTION_SYNTAX CURLE_SETOPT_OPTION_SYNTAX

#define CURLE_SSL_PEER_CERTIFICATE CURLE_PEER_FAILED_VERIFICATION

#define CURLE_OBSOLETE CURLE_OBSOLETE50 
#define CURLE_BAD_PASSWORD_ENTERED CURLE_OBSOLETE46
#define CURLE_BAD_CALLING_ORDER CURLE_OBSOLETE44
#define CURLE_FTP_USER_PASSWORD_INCORRECT CURLE_OBSOLETE10
#define CURLE_FTP_CANT_RECONNECT CURLE_OBSOLETE16
#define CURLE_FTP_COULDNT_GET_SIZE CURLE_OBSOLETE32
#define CURLE_FTP_COULDNT_SET_ASCII CURLE_OBSOLETE29
#define CURLE_FTP_WEIRD_USER_REPLY CURLE_OBSOLETE12
#define CURLE_FTP_WRITE_ERROR CURLE_OBSOLETE20
#define CURLE_LIBRARY_NOT_FOUND CURLE_OBSOLETE40
#define CURLE_MALFORMAT_USER CURLE_OBSOLETE24
#define CURLE_SHARE_IN_USE CURLE_OBSOLETE57
#define CURLE_URL_MALFORMAT_USER CURLE_NOT_BUILT_IN

#define CURLE_FTP_ACCESS_DENIED CURLE_REMOTE_ACCESS_DENIED
#define CURLE_FTP_COULDNT_SET_BINARY CURLE_FTP_COULDNT_SET_TYPE
#define CURLE_FTP_QUOTE_ERROR CURLE_QUOTE_ERROR
#define CURLE_TFTP_DISKFULL CURLE_REMOTE_DISK_FULL
#define CURLE_TFTP_EXISTS CURLE_REMOTE_FILE_EXISTS
#define CURLE_HTTP_RANGE_ERROR CURLE_RANGE_ERROR
#define CURLE_FTP_SSL_FAILED CURLE_USE_SSL_FAILED

    

#define CURLE_OPERATION_TIMEOUTED CURLE_OPERATION_TIMEDOUT
#define CURLE_HTTP_NOT_FOUND CURLE_HTTP_RETURNED_ERROR
#define CURLE_HTTP_PORT_FAILED CURLE_INTERFACE_FAILED
#define CURLE_FTP_COULDNT_STOR_FILE CURLE_UPLOAD_FAILED
#define CURLE_FTP_PARTIAL_FILE CURLE_PARTIAL_FILE
#define CURLE_FTP_BAD_DOWNLOAD_RESUME CURLE_BAD_DOWNLOAD_RESUME
#define CURLE_LDAP_INVALID_URL CURLE_OBSOLETE62
#define CURLE_CONV_REQD CURLE_OBSOLETE76
#define CURLE_CONV_FAILED CURLE_OBSOLETE75

#define CURLE_ALREADY_COMPLETE 99999

#define CURLOPT_FILE CURLOPT_WRITEDATA  
#define CURLOPT_INFILE CURLOPT_READDATA 
#define CURLOPT_WRITEHEADER CURLOPT_HEADERDATA

#define CURLOPT_WRITEINFO CURLOPT_OBSOLETE40
#define CURLOPT_CLOSEPOLICY CURLOPT_OBSOLETE72

#endif 

    

    typedef enum
    {
        CURLPX_OK,
        CURLPX_BAD_ADDRESS_TYPE,
        CURLPX_BAD_VERSION,
        CURLPX_CLOSED,
        CURLPX_GSSAPI,
        CURLPX_GSSAPI_PERMSG,
        CURLPX_GSSAPI_PROTECTION,
        CURLPX_IDENTD,
        CURLPX_IDENTD_DIFFER,
        CURLPX_LONG_HOSTNAME,
        CURLPX_LONG_PASSWD,
        CURLPX_LONG_USER,
        CURLPX_NO_AUTH,
        CURLPX_RECV_ADDRESS,
        CURLPX_RECV_AUTH,
        CURLPX_RECV_CONNECT,
        CURLPX_RECV_REQACK,
        CURLPX_REPLY_ADDRESS_TYPE_NOT_SUPPORTED,
        CURLPX_REPLY_COMMAND_NOT_SUPPORTED,
        CURLPX_REPLY_CONNECTION_REFUSED,
        CURLPX_REPLY_GENERAL_SERVER_FAILURE,
        CURLPX_REPLY_HOST_UNREACHABLE,
        CURLPX_REPLY_NETWORK_UNREACHABLE,
        CURLPX_REPLY_NOT_ALLOWED,
        CURLPX_REPLY_TTL_EXPIRED,
        CURLPX_REPLY_UNASSIGNED,
        CURLPX_REQUEST_FAILED,
        CURLPX_RESOLVE_HOST,
        CURLPX_SEND_AUTH,
        CURLPX_SEND_CONNECT,
        CURLPX_SEND_REQUEST,
        CURLPX_UNKNOWN_FAIL,
        CURLPX_UNKNOWN_MODE,
        CURLPX_USER_REJECTED,
        CURLPX_LAST 
    } CURLproxycode;

    
    typedef CURLcode (*curl_conv_callback)(char* buffer, size_t length);

    typedef CURLcode (*curl_ssl_ctx_callback)(CURL* curl,    
                                              void* ssl_ctx, 

                                              void* userptr);

    typedef enum
    {
        CURLPROXY_HTTP = 0,           

        CURLPROXY_HTTP_1_0 = 1,       

        CURLPROXY_HTTPS = 2,          
        CURLPROXY_SOCKS4 = 4,         

        CURLPROXY_SOCKS5 = 5,         
        CURLPROXY_SOCKS4A = 6,        
        CURLPROXY_SOCKS5_HOSTNAME = 7 

    } curl_proxytype;                 

    

#define CURLAUTH_NONE ((unsigned long)0)
#define CURLAUTH_BASIC (((unsigned long)1) << 0)
#define CURLAUTH_DIGEST (((unsigned long)1) << 1)
#define CURLAUTH_NEGOTIATE (((unsigned long)1) << 2)

#define CURLAUTH_GSSNEGOTIATE CURLAUTH_NEGOTIATE

#define CURLAUTH_GSSAPI CURLAUTH_NEGOTIATE
#define CURLAUTH_NTLM (((unsigned long)1) << 3)
#define CURLAUTH_DIGEST_IE (((unsigned long)1) << 4)
#define CURLAUTH_NTLM_WB (((unsigned long)1) << 5)
#define CURLAUTH_BEARER (((unsigned long)1) << 6)
#define CURLAUTH_AWS_SIGV4 (((unsigned long)1) << 7)
#define CURLAUTH_ONLY (((unsigned long)1) << 31)
#define CURLAUTH_ANY (~CURLAUTH_DIGEST_IE)
#define CURLAUTH_ANYSAFE (~(CURLAUTH_BASIC | CURLAUTH_DIGEST_IE))

#define CURLSSH_AUTH_ANY ~0             
#define CURLSSH_AUTH_NONE 0             
#define CURLSSH_AUTH_PUBLICKEY (1 << 0) 
#define CURLSSH_AUTH_PASSWORD (1 << 1)  
#define CURLSSH_AUTH_HOST (1 << 2)      
#define CURLSSH_AUTH_KEYBOARD (1 << 3)  
#define CURLSSH_AUTH_AGENT (1 << 4)     
#define CURLSSH_AUTH_GSSAPI (1 << 5)    
#define CURLSSH_AUTH_DEFAULT CURLSSH_AUTH_ANY

#define CURLGSSAPI_DELEGATION_NONE 0               
#define CURLGSSAPI_DELEGATION_POLICY_FLAG (1 << 0) 
#define CURLGSSAPI_DELEGATION_FLAG (1 << 1)        

#define CURL_ERROR_SIZE 256

    enum curl_khtype
    {
        CURLKHTYPE_UNKNOWN,
        CURLKHTYPE_RSA1,
        CURLKHTYPE_RSA,
        CURLKHTYPE_DSS,
        CURLKHTYPE_ECDSA,
        CURLKHTYPE_ED25519
    };

    struct curl_khkey
    {
        const char* key; 

        size_t len;
        enum curl_khtype keytype;
    };

    

    enum curl_khstat
    {
        CURLKHSTAT_FINE_ADD_TO_FILE,
        CURLKHSTAT_FINE,
        CURLKHSTAT_REJECT,       
        CURLKHSTAT_DEFER,        

        CURLKHSTAT_FINE_REPLACE, 
        CURLKHSTAT_LAST          
    };

    
    enum curl_khmatch
    {
        CURLKHMATCH_OK,       
        CURLKHMATCH_MISMATCH, 
        CURLKHMATCH_MISSING,  
        CURLKHMATCH_LAST      
    };

    typedef int (*curl_sshkeycallback)(CURL* easy,                        
                                       const struct curl_khkey* knownkey, 
                                       const struct curl_khkey* foundkey, 
                                       enum curl_khmatch,                 
                                       void* clientp);                    
                                                                          

    typedef int (*curl_sshhostkeycallback)(void* clientp,   
                                                            
                                           int keytype,     
                                           const char* key, 
                                           size_t keylen);  
                                                            
                                                            

    
    typedef enum
    {
        CURLUSESSL_NONE,    
        CURLUSESSL_TRY,     
        CURLUSESSL_CONTROL, 
        CURLUSESSL_ALL,     
        CURLUSESSL_LAST     
    } curl_usessl;

#define CURLSSLOPT_ALLOW_BEAST (1 << 0)

#define CURLSSLOPT_NO_REVOKE (1 << 1)

#define CURLSSLOPT_NO_PARTIALCHAIN (1 << 2)

#define CURLSSLOPT_REVOKE_BEST_EFFORT (1 << 3)

#define CURLSSLOPT_NATIVE_CA (1 << 4)

#define CURLSSLOPT_AUTO_CLIENT_CERT (1 << 5)

#define CURL_HET_DEFAULT 200L

#define CURL_UPKEEP_INTERVAL_DEFAULT 60000L

#ifndef CURL_NO_OLDIES 

    
    

#define CURLFTPSSL_NONE CURLUSESSL_NONE
#define CURLFTPSSL_TRY CURLUSESSL_TRY
#define CURLFTPSSL_CONTROL CURLUSESSL_CONTROL
#define CURLFTPSSL_ALL CURLUSESSL_ALL
#define CURLFTPSSL_LAST CURLUSESSL_LAST
#define curl_ftpssl curl_usessl
#endif 

    
    typedef enum
    {
        CURLFTPSSL_CCC_NONE,    
        CURLFTPSSL_CCC_PASSIVE, 
        CURLFTPSSL_CCC_ACTIVE,  
        CURLFTPSSL_CCC_LAST     
    } curl_ftpccc;

    
    typedef enum
    {
        CURLFTPAUTH_DEFAULT, 
        CURLFTPAUTH_SSL,     
        CURLFTPAUTH_TLS,     
        CURLFTPAUTH_LAST     
    } curl_ftpauth;

    
    typedef enum
    {
        CURLFTP_CREATE_DIR_NONE,  
        CURLFTP_CREATE_DIR,       

        CURLFTP_CREATE_DIR_RETRY, 

        CURLFTP_CREATE_DIR_LAST   
    } curl_ftpcreatedir;

    
    typedef enum
    {
        CURLFTPMETHOD_DEFAULT,   
        CURLFTPMETHOD_MULTICWD,  
        CURLFTPMETHOD_NOCWD,     
        CURLFTPMETHOD_SINGLECWD, 
        CURLFTPMETHOD_LAST       
    } curl_ftpmethod;

#define CURLHEADER_UNIFIED 0
#define CURLHEADER_SEPARATE (1 << 0)

#define CURLALTSVC_READONLYFILE (1 << 2)
#define CURLALTSVC_H1 (1 << 3)
#define CURLALTSVC_H2 (1 << 4)
#define CURLALTSVC_H3 (1 << 5)

    struct curl_hstsentry
    {
        char* name;
        size_t namelen;
        unsigned int includeSubDomains : 1;
        char expire[18]; 
    };

    struct curl_index
    {
        size_t index; 
        size_t total; 
    };

    typedef enum
    {
        CURLSTS_OK,
        CURLSTS_DONE,
        CURLSTS_FAIL
    } CURLSTScode;

    typedef CURLSTScode (*curl_hstsread_callback)(CURL* easy, struct curl_hstsentry* e, void* userp);
    typedef CURLSTScode (*curl_hstswrite_callback)(CURL* easy, struct curl_hstsentry* e, struct curl_index* i, void* userp);

#define CURLHSTS_ENABLE (long)(1 << 0)
#define CURLHSTS_READONLYFILE (long)(1 << 1)

#define CURLPROTO_HTTP (1 << 0)
#define CURLPROTO_HTTPS (1 << 1)
#define CURLPROTO_FTP (1 << 2)
#define CURLPROTO_FTPS (1 << 3)
#define CURLPROTO_SCP (1 << 4)
#define CURLPROTO_SFTP (1 << 5)
#define CURLPROTO_TELNET (1 << 6)
#define CURLPROTO_LDAP (1 << 7)
#define CURLPROTO_LDAPS (1 << 8)
#define CURLPROTO_DICT (1 << 9)
#define CURLPROTO_FILE (1 << 10)
#define CURLPROTO_TFTP (1 << 11)
#define CURLPROTO_IMAP (1 << 12)
#define CURLPROTO_IMAPS (1 << 13)
#define CURLPROTO_POP3 (1 << 14)
#define CURLPROTO_POP3S (1 << 15)
#define CURLPROTO_SMTP (1 << 16)
#define CURLPROTO_SMTPS (1 << 17)
#define CURLPROTO_RTSP (1 << 18)
#define CURLPROTO_RTMP (1 << 19)
#define CURLPROTO_RTMPT (1 << 20)
#define CURLPROTO_RTMPE (1 << 21)
#define CURLPROTO_RTMPTE (1 << 22)
#define CURLPROTO_RTMPS (1 << 23)
#define CURLPROTO_RTMPTS (1 << 24)
#define CURLPROTO_GOPHER (1 << 25)
#define CURLPROTO_SMB (1 << 26)
#define CURLPROTO_SMBS (1 << 27)
#define CURLPROTO_MQTT (1 << 28)
#define CURLPROTO_GOPHERS (1 << 29)
#define CURLPROTO_ALL (~0) 

#define CURLOPTTYPE_LONG 0
#define CURLOPTTYPE_OBJECTPOINT 10000
#define CURLOPTTYPE_FUNCTIONPOINT 20000
#define CURLOPTTYPE_OFF_T 30000
#define CURLOPTTYPE_BLOB 40000

    

#define CURLOPT(na, t, nu) na = t + nu

#define CURLOPTTYPE_STRINGPOINT CURLOPTTYPE_OBJECTPOINT

#define CURLOPTTYPE_SLISTPOINT CURLOPTTYPE_OBJECTPOINT

#define CURLOPTTYPE_CBPOINT CURLOPTTYPE_OBJECTPOINT

#define CURLOPTTYPE_VALUES CURLOPTTYPE_LONG

    

    typedef enum
    {
        
        CURLOPT(CURLOPT_WRITEDATA, CURLOPTTYPE_CBPOINT, 1),

        
        CURLOPT(CURLOPT_URL, CURLOPTTYPE_STRINGPOINT, 2),

        
        CURLOPT(CURLOPT_PORT, CURLOPTTYPE_LONG, 3),

        
        CURLOPT(CURLOPT_PROXY, CURLOPTTYPE_STRINGPOINT, 4),

        
        CURLOPT(CURLOPT_USERPWD, CURLOPTTYPE_STRINGPOINT, 5),

        
        CURLOPT(CURLOPT_PROXYUSERPWD, CURLOPTTYPE_STRINGPOINT, 6),

        
        CURLOPT(CURLOPT_RANGE, CURLOPTTYPE_STRINGPOINT, 7),

        

        
        CURLOPT(CURLOPT_READDATA, CURLOPTTYPE_CBPOINT, 9),

        

        CURLOPT(CURLOPT_ERRORBUFFER, CURLOPTTYPE_OBJECTPOINT, 10),

        

        CURLOPT(CURLOPT_WRITEFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 11),

        

        CURLOPT(CURLOPT_READFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 12),

        
        CURLOPT(CURLOPT_TIMEOUT, CURLOPTTYPE_LONG, 13),

        

        CURLOPT(CURLOPT_INFILESIZE, CURLOPTTYPE_LONG, 14),

        
        CURLOPT(CURLOPT_POSTFIELDS, CURLOPTTYPE_OBJECTPOINT, 15),

        
        CURLOPT(CURLOPT_REFERER, CURLOPTTYPE_STRINGPOINT, 16),

        

        CURLOPT(CURLOPT_FTPPORT, CURLOPTTYPE_STRINGPOINT, 17),

        
        CURLOPT(CURLOPT_USERAGENT, CURLOPTTYPE_STRINGPOINT, 18),

        

        
        CURLOPT(CURLOPT_LOW_SPEED_LIMIT, CURLOPTTYPE_LONG, 19),

        
        CURLOPT(CURLOPT_LOW_SPEED_TIME, CURLOPTTYPE_LONG, 20),

        

        CURLOPT(CURLOPT_RESUME_FROM, CURLOPTTYPE_LONG, 21),

        
        CURLOPT(CURLOPT_COOKIE, CURLOPTTYPE_STRINGPOINT, 22),

        

        CURLOPT(CURLOPT_HTTPHEADER, CURLOPTTYPE_SLISTPOINT, 23),

        
        CURLOPT(CURLOPT_HTTPPOST, CURLOPTTYPE_OBJECTPOINT, 24),

        
        CURLOPT(CURLOPT_SSLCERT, CURLOPTTYPE_STRINGPOINT, 25),

        
        CURLOPT(CURLOPT_KEYPASSWD, CURLOPTTYPE_STRINGPOINT, 26),

        
        CURLOPT(CURLOPT_CRLF, CURLOPTTYPE_LONG, 27),

        
        CURLOPT(CURLOPT_QUOTE, CURLOPTTYPE_SLISTPOINT, 28),

        

        CURLOPT(CURLOPT_HEADERDATA, CURLOPTTYPE_CBPOINT, 29),

        

        CURLOPT(CURLOPT_COOKIEFILE, CURLOPTTYPE_STRINGPOINT, 31),

        

        CURLOPT(CURLOPT_SSLVERSION, CURLOPTTYPE_VALUES, 32),

        
        CURLOPT(CURLOPT_TIMECONDITION, CURLOPTTYPE_VALUES, 33),

        

        CURLOPT(CURLOPT_TIMEVALUE, CURLOPTTYPE_LONG, 34),

        

        

        CURLOPT(CURLOPT_CUSTOMREQUEST, CURLOPTTYPE_STRINGPOINT, 36),

        
        CURLOPT(CURLOPT_STDERR, CURLOPTTYPE_OBJECTPOINT, 37),

        

        
        CURLOPT(CURLOPT_POSTQUOTE, CURLOPTTYPE_SLISTPOINT, 39),

        
        CURLOPT(CURLOPT_OBSOLETE40, CURLOPTTYPE_OBJECTPOINT, 40),

        
        CURLOPT(CURLOPT_VERBOSE, CURLOPTTYPE_LONG, 41),

        
        CURLOPT(CURLOPT_HEADER, CURLOPTTYPE_LONG, 42),

        
        CURLOPT(CURLOPT_NOPROGRESS, CURLOPTTYPE_LONG, 43),

        
        CURLOPT(CURLOPT_NOBODY, CURLOPTTYPE_LONG, 44),

        
        CURLOPT(CURLOPT_FAILONERROR, CURLOPTTYPE_LONG, 45),

        
        CURLOPT(CURLOPT_UPLOAD, CURLOPTTYPE_LONG, 46),

        
        CURLOPT(CURLOPT_POST, CURLOPTTYPE_LONG, 47),

        
        CURLOPT(CURLOPT_DIRLISTONLY, CURLOPTTYPE_LONG, 48),

        
        CURLOPT(CURLOPT_APPEND, CURLOPTTYPE_LONG, 50),

        

        CURLOPT(CURLOPT_NETRC, CURLOPTTYPE_VALUES, 51),

        
        CURLOPT(CURLOPT_FOLLOWLOCATION, CURLOPTTYPE_LONG, 52),

        
        CURLOPT(CURLOPT_TRANSFERTEXT, CURLOPTTYPE_LONG, 53),

        
        CURLOPT(CURLOPT_PUT, CURLOPTTYPE_LONG, 54),

        

        

        CURLOPT(CURLOPT_PROGRESSFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 56),

        

        CURLOPT(CURLOPT_XFERINFODATA, CURLOPTTYPE_CBPOINT, 57),
#define CURLOPT_PROGRESSDATA CURLOPT_XFERINFODATA

        
        CURLOPT(CURLOPT_AUTOREFERER, CURLOPTTYPE_LONG, 58),

        

        CURLOPT(CURLOPT_PROXYPORT, CURLOPTTYPE_LONG, 59),

        
        CURLOPT(CURLOPT_POSTFIELDSIZE, CURLOPTTYPE_LONG, 60),

        
        CURLOPT(CURLOPT_HTTPPROXYTUNNEL, CURLOPTTYPE_LONG, 61),

        
        CURLOPT(CURLOPT_INTERFACE, CURLOPTTYPE_STRINGPOINT, 62),

        

        CURLOPT(CURLOPT_KRBLEVEL, CURLOPTTYPE_STRINGPOINT, 63),

        
        CURLOPT(CURLOPT_SSL_VERIFYPEER, CURLOPTTYPE_LONG, 64),

        

        CURLOPT(CURLOPT_CAINFO, CURLOPTTYPE_STRINGPOINT, 65),

        
        

        
        CURLOPT(CURLOPT_MAXREDIRS, CURLOPTTYPE_LONG, 68),

        

        CURLOPT(CURLOPT_FILETIME, CURLOPTTYPE_LONG, 69),

        
        CURLOPT(CURLOPT_TELNETOPTIONS, CURLOPTTYPE_SLISTPOINT, 70),

        
        CURLOPT(CURLOPT_MAXCONNECTS, CURLOPTTYPE_LONG, 71),

        
        CURLOPT(CURLOPT_OBSOLETE72, CURLOPTTYPE_LONG, 72),

        

        

        CURLOPT(CURLOPT_FRESH_CONNECT, CURLOPTTYPE_LONG, 74),

        

        CURLOPT(CURLOPT_FORBID_REUSE, CURLOPTTYPE_LONG, 75),

        

        CURLOPT(CURLOPT_RANDOM_FILE, CURLOPTTYPE_STRINGPOINT, 76),

        
        CURLOPT(CURLOPT_EGDSOCKET, CURLOPTTYPE_STRINGPOINT, 77),

        

        CURLOPT(CURLOPT_CONNECTTIMEOUT, CURLOPTTYPE_LONG, 78),

        

        CURLOPT(CURLOPT_HEADERFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 79),

        

        CURLOPT(CURLOPT_HTTPGET, CURLOPTTYPE_LONG, 80),

        

        CURLOPT(CURLOPT_SSL_VERIFYHOST, CURLOPTTYPE_LONG, 81),

        

        CURLOPT(CURLOPT_COOKIEJAR, CURLOPTTYPE_STRINGPOINT, 82),

        
        CURLOPT(CURLOPT_SSL_CIPHER_LIST, CURLOPTTYPE_STRINGPOINT, 83),

        

        CURLOPT(CURLOPT_HTTP_VERSION, CURLOPTTYPE_VALUES, 84),

        

        CURLOPT(CURLOPT_FTP_USE_EPSV, CURLOPTTYPE_LONG, 85),

        
        CURLOPT(CURLOPT_SSLCERTTYPE, CURLOPTTYPE_STRINGPOINT, 86),

        
        CURLOPT(CURLOPT_SSLKEY, CURLOPTTYPE_STRINGPOINT, 87),

        
        CURLOPT(CURLOPT_SSLKEYTYPE, CURLOPTTYPE_STRINGPOINT, 88),

        
        CURLOPT(CURLOPT_SSLENGINE, CURLOPTTYPE_STRINGPOINT, 89),

        

        CURLOPT(CURLOPT_SSLENGINE_DEFAULT, CURLOPTTYPE_LONG, 90),

        
        
        CURLOPT(CURLOPT_DNS_USE_GLOBAL_CACHE, CURLOPTTYPE_LONG, 91),

        
        CURLOPT(CURLOPT_DNS_CACHE_TIMEOUT, CURLOPTTYPE_LONG, 92),

        
        CURLOPT(CURLOPT_PREQUOTE, CURLOPTTYPE_SLISTPOINT, 93),

        
        CURLOPT(CURLOPT_DEBUGFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 94),

        
        CURLOPT(CURLOPT_DEBUGDATA, CURLOPTTYPE_CBPOINT, 95),

        
        CURLOPT(CURLOPT_COOKIESESSION, CURLOPTTYPE_LONG, 96),

        

        CURLOPT(CURLOPT_CAPATH, CURLOPTTYPE_STRINGPOINT, 97),

        
        CURLOPT(CURLOPT_BUFFERSIZE, CURLOPTTYPE_LONG, 98),

        

        CURLOPT(CURLOPT_NOSIGNAL, CURLOPTTYPE_LONG, 99),

        
        CURLOPT(CURLOPT_SHARE, CURLOPTTYPE_OBJECTPOINT, 100),

        

        CURLOPT(CURLOPT_PROXYTYPE, CURLOPTTYPE_VALUES, 101),

        

        CURLOPT(CURLOPT_ACCEPT_ENCODING, CURLOPTTYPE_STRINGPOINT, 102),

        
        CURLOPT(CURLOPT_PRIVATE, CURLOPTTYPE_OBJECTPOINT, 103),

        
        CURLOPT(CURLOPT_HTTP200ALIASES, CURLOPTTYPE_SLISTPOINT, 104),

        

        CURLOPT(CURLOPT_UNRESTRICTED_AUTH, CURLOPTTYPE_LONG, 105),

        

        CURLOPT(CURLOPT_FTP_USE_EPRT, CURLOPTTYPE_LONG, 106),

        

        CURLOPT(CURLOPT_HTTPAUTH, CURLOPTTYPE_VALUES, 107),

        

        CURLOPT(CURLOPT_SSL_CTX_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 108),

        

        CURLOPT(CURLOPT_SSL_CTX_DATA, CURLOPTTYPE_CBPOINT, 109),

        

        CURLOPT(CURLOPT_FTP_CREATE_MISSING_DIRS, CURLOPTTYPE_LONG, 110),

        

        CURLOPT(CURLOPT_PROXYAUTH, CURLOPTTYPE_VALUES, 111),

        

        CURLOPT(CURLOPT_SERVER_RESPONSE_TIMEOUT, CURLOPTTYPE_LONG, 112),

        

        CURLOPT(CURLOPT_IPRESOLVE, CURLOPTTYPE_VALUES, 113),

        

        CURLOPT(CURLOPT_MAXFILESIZE, CURLOPTTYPE_LONG, 114),

        

        CURLOPT(CURLOPT_INFILESIZE_LARGE, CURLOPTTYPE_OFF_T, 115),

        

        CURLOPT(CURLOPT_RESUME_FROM_LARGE, CURLOPTTYPE_OFF_T, 116),

        

        CURLOPT(CURLOPT_MAXFILESIZE_LARGE, CURLOPTTYPE_OFF_T, 117),

        

        CURLOPT(CURLOPT_NETRC_FILE, CURLOPTTYPE_STRINGPOINT, 118),

        

        CURLOPT(CURLOPT_USE_SSL, CURLOPTTYPE_VALUES, 119),

        
        CURLOPT(CURLOPT_POSTFIELDSIZE_LARGE, CURLOPTTYPE_OFF_T, 120),

        
        CURLOPT(CURLOPT_TCP_NODELAY, CURLOPTTYPE_LONG, 121),

        
        
        
        
        
        
        

        

        CURLOPT(CURLOPT_FTPSSLAUTH, CURLOPTTYPE_VALUES, 129),

        CURLOPT(CURLOPT_IOCTLFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 130),
        CURLOPT(CURLOPT_IOCTLDATA, CURLOPTTYPE_CBPOINT, 131),

        
        

        

        CURLOPT(CURLOPT_FTP_ACCOUNT, CURLOPTTYPE_STRINGPOINT, 134),

        
        CURLOPT(CURLOPT_COOKIELIST, CURLOPTTYPE_STRINGPOINT, 135),

        
        CURLOPT(CURLOPT_IGNORE_CONTENT_LENGTH, CURLOPTTYPE_LONG, 136),

        

        CURLOPT(CURLOPT_FTP_SKIP_PASV_IP, CURLOPTTYPE_LONG, 137),

        

        CURLOPT(CURLOPT_FTP_FILEMETHOD, CURLOPTTYPE_VALUES, 138),

        
        CURLOPT(CURLOPT_LOCALPORT, CURLOPTTYPE_LONG, 139),

        

        CURLOPT(CURLOPT_LOCALPORTRANGE, CURLOPTTYPE_LONG, 140),

        

        CURLOPT(CURLOPT_CONNECT_ONLY, CURLOPTTYPE_LONG, 141),

        

        CURLOPT(CURLOPT_CONV_FROM_NETWORK_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 142),

        

        CURLOPT(CURLOPT_CONV_TO_NETWORK_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 143),

        

        CURLOPT(CURLOPT_CONV_FROM_UTF8_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 144),

        
        
        CURLOPT(CURLOPT_MAX_SEND_SPEED_LARGE, CURLOPTTYPE_OFF_T, 145),
        CURLOPT(CURLOPT_MAX_RECV_SPEED_LARGE, CURLOPTTYPE_OFF_T, 146),

        
        CURLOPT(CURLOPT_FTP_ALTERNATIVE_TO_USER, CURLOPTTYPE_STRINGPOINT, 147),

        
        CURLOPT(CURLOPT_SOCKOPTFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 148),
        CURLOPT(CURLOPT_SOCKOPTDATA, CURLOPTTYPE_CBPOINT, 149),

        

        CURLOPT(CURLOPT_SSL_SESSIONID_CACHE, CURLOPTTYPE_LONG, 150),

        
        CURLOPT(CURLOPT_SSH_AUTH_TYPES, CURLOPTTYPE_VALUES, 151),

        
        CURLOPT(CURLOPT_SSH_PUBLIC_KEYFILE, CURLOPTTYPE_STRINGPOINT, 152),
        CURLOPT(CURLOPT_SSH_PRIVATE_KEYFILE, CURLOPTTYPE_STRINGPOINT, 153),

        
        CURLOPT(CURLOPT_FTP_SSL_CCC, CURLOPTTYPE_LONG, 154),

        
        CURLOPT(CURLOPT_TIMEOUT_MS, CURLOPTTYPE_LONG, 155),
        CURLOPT(CURLOPT_CONNECTTIMEOUT_MS, CURLOPTTYPE_LONG, 156),

        

        CURLOPT(CURLOPT_HTTP_TRANSFER_DECODING, CURLOPTTYPE_LONG, 157),
        CURLOPT(CURLOPT_HTTP_CONTENT_DECODING, CURLOPTTYPE_LONG, 158),

        

        CURLOPT(CURLOPT_NEW_FILE_PERMS, CURLOPTTYPE_LONG, 159),
        CURLOPT(CURLOPT_NEW_DIRECTORY_PERMS, CURLOPTTYPE_LONG, 160),

        

        CURLOPT(CURLOPT_POSTREDIR, CURLOPTTYPE_VALUES, 161),

        
        CURLOPT(CURLOPT_SSH_HOST_PUBLIC_KEY_MD5, CURLOPTTYPE_STRINGPOINT, 162),

        

        CURLOPT(CURLOPT_OPENSOCKETFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 163),
        CURLOPT(CURLOPT_OPENSOCKETDATA, CURLOPTTYPE_CBPOINT, 164),

        
        CURLOPT(CURLOPT_COPYPOSTFIELDS, CURLOPTTYPE_OBJECTPOINT, 165),

        
        CURLOPT(CURLOPT_PROXY_TRANSFER_MODE, CURLOPTTYPE_LONG, 166),

        
        CURLOPT(CURLOPT_SEEKFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 167),
        CURLOPT(CURLOPT_SEEKDATA, CURLOPTTYPE_CBPOINT, 168),

        
        CURLOPT(CURLOPT_CRLFILE, CURLOPTTYPE_STRINGPOINT, 169),

        
        CURLOPT(CURLOPT_ISSUERCERT, CURLOPTTYPE_STRINGPOINT, 170),

        
        CURLOPT(CURLOPT_ADDRESS_SCOPE, CURLOPTTYPE_LONG, 171),

        

        CURLOPT(CURLOPT_CERTINFO, CURLOPTTYPE_LONG, 172),

        
        CURLOPT(CURLOPT_USERNAME, CURLOPTTYPE_STRINGPOINT, 173),
        CURLOPT(CURLOPT_PASSWORD, CURLOPTTYPE_STRINGPOINT, 174),

        
        CURLOPT(CURLOPT_PROXYUSERNAME, CURLOPTTYPE_STRINGPOINT, 175),
        CURLOPT(CURLOPT_PROXYPASSWORD, CURLOPTTYPE_STRINGPOINT, 176),

        

        CURLOPT(CURLOPT_NOPROXY, CURLOPTTYPE_STRINGPOINT, 177),

        
        CURLOPT(CURLOPT_TFTP_BLKSIZE, CURLOPTTYPE_LONG, 178),

        
        
        CURLOPT(CURLOPT_SOCKS5_GSSAPI_SERVICE, CURLOPTTYPE_STRINGPOINT, 179),

        
        CURLOPT(CURLOPT_SOCKS5_GSSAPI_NEC, CURLOPTTYPE_LONG, 180),

        

        CURLOPT(CURLOPT_PROTOCOLS, CURLOPTTYPE_LONG, 181),

        

        CURLOPT(CURLOPT_REDIR_PROTOCOLS, CURLOPTTYPE_LONG, 182),

        
        CURLOPT(CURLOPT_SSH_KNOWNHOSTS, CURLOPTTYPE_STRINGPOINT, 183),

        

        CURLOPT(CURLOPT_SSH_KEYFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 184),

        
        CURLOPT(CURLOPT_SSH_KEYDATA, CURLOPTTYPE_CBPOINT, 185),

        
        CURLOPT(CURLOPT_MAIL_FROM, CURLOPTTYPE_STRINGPOINT, 186),

        
        CURLOPT(CURLOPT_MAIL_RCPT, CURLOPTTYPE_SLISTPOINT, 187),

        
        CURLOPT(CURLOPT_FTP_USE_PRET, CURLOPTTYPE_LONG, 188),

        
        CURLOPT(CURLOPT_RTSP_REQUEST, CURLOPTTYPE_VALUES, 189),

        
        CURLOPT(CURLOPT_RTSP_SESSION_ID, CURLOPTTYPE_STRINGPOINT, 190),

        
        CURLOPT(CURLOPT_RTSP_STREAM_URI, CURLOPTTYPE_STRINGPOINT, 191),

        
        CURLOPT(CURLOPT_RTSP_TRANSPORT, CURLOPTTYPE_STRINGPOINT, 192),

        
        CURLOPT(CURLOPT_RTSP_CLIENT_CSEQ, CURLOPTTYPE_LONG, 193),

        
        CURLOPT(CURLOPT_RTSP_SERVER_CSEQ, CURLOPTTYPE_LONG, 194),

        
        CURLOPT(CURLOPT_INTERLEAVEDATA, CURLOPTTYPE_CBPOINT, 195),

        
        CURLOPT(CURLOPT_INTERLEAVEFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 196),

        
        CURLOPT(CURLOPT_WILDCARDMATCH, CURLOPTTYPE_LONG, 197),

        

        CURLOPT(CURLOPT_CHUNK_BGN_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 198),

        

        CURLOPT(CURLOPT_CHUNK_END_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 199),

        
        CURLOPT(CURLOPT_FNMATCH_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 200),

        
        CURLOPT(CURLOPT_CHUNK_DATA, CURLOPTTYPE_CBPOINT, 201),

        
        CURLOPT(CURLOPT_FNMATCH_DATA, CURLOPTTYPE_CBPOINT, 202),

        
        CURLOPT(CURLOPT_RESOLVE, CURLOPTTYPE_SLISTPOINT, 203),

        
        CURLOPT(CURLOPT_TLSAUTH_USERNAME, CURLOPTTYPE_STRINGPOINT, 204),

        
        CURLOPT(CURLOPT_TLSAUTH_PASSWORD, CURLOPTTYPE_STRINGPOINT, 205),

        
        CURLOPT(CURLOPT_TLSAUTH_TYPE, CURLOPTTYPE_STRINGPOINT, 206),

        

        CURLOPT(CURLOPT_TRANSFER_ENCODING, CURLOPTTYPE_LONG, 207),

        

        CURLOPT(CURLOPT_CLOSESOCKETFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 208),
        CURLOPT(CURLOPT_CLOSESOCKETDATA, CURLOPTTYPE_CBPOINT, 209),

        
        CURLOPT(CURLOPT_GSSAPI_DELEGATION, CURLOPTTYPE_VALUES, 210),

        
        CURLOPT(CURLOPT_DNS_SERVERS, CURLOPTTYPE_STRINGPOINT, 211),

        

        CURLOPT(CURLOPT_ACCEPTTIMEOUT_MS, CURLOPTTYPE_LONG, 212),

        
        CURLOPT(CURLOPT_TCP_KEEPALIVE, CURLOPTTYPE_LONG, 213),

        
        CURLOPT(CURLOPT_TCP_KEEPIDLE, CURLOPTTYPE_LONG, 214),
        CURLOPT(CURLOPT_TCP_KEEPINTVL, CURLOPTTYPE_LONG, 215),

        
        CURLOPT(CURLOPT_SSL_OPTIONS, CURLOPTTYPE_VALUES, 216),

        
        CURLOPT(CURLOPT_MAIL_AUTH, CURLOPTTYPE_STRINGPOINT, 217),

        
        CURLOPT(CURLOPT_SASL_IR, CURLOPTTYPE_LONG, 218),

        

        CURLOPT(CURLOPT_XFERINFOFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 219),

        
        CURLOPT(CURLOPT_XOAUTH2_BEARER, CURLOPTTYPE_STRINGPOINT, 220),

        

        CURLOPT(CURLOPT_DNS_INTERFACE, CURLOPTTYPE_STRINGPOINT, 221),

        

        CURLOPT(CURLOPT_DNS_LOCAL_IP4, CURLOPTTYPE_STRINGPOINT, 222),

        

        CURLOPT(CURLOPT_DNS_LOCAL_IP6, CURLOPTTYPE_STRINGPOINT, 223),

        
        CURLOPT(CURLOPT_LOGIN_OPTIONS, CURLOPTTYPE_STRINGPOINT, 224),

        
        CURLOPT(CURLOPT_SSL_ENABLE_NPN, CURLOPTTYPE_LONG, 225),

        
        CURLOPT(CURLOPT_SSL_ENABLE_ALPN, CURLOPTTYPE_LONG, 226),

        

        CURLOPT(CURLOPT_EXPECT_100_TIMEOUT_MS, CURLOPTTYPE_LONG, 227),

        

        CURLOPT(CURLOPT_PROXYHEADER, CURLOPTTYPE_SLISTPOINT, 228),

        
        CURLOPT(CURLOPT_HEADEROPT, CURLOPTTYPE_VALUES, 229),

        

        CURLOPT(CURLOPT_PINNEDPUBLICKEY, CURLOPTTYPE_STRINGPOINT, 230),

        
        CURLOPT(CURLOPT_UNIX_SOCKET_PATH, CURLOPTTYPE_STRINGPOINT, 231),

        
        CURLOPT(CURLOPT_SSL_VERIFYSTATUS, CURLOPTTYPE_LONG, 232),

        
        CURLOPT(CURLOPT_SSL_FALSESTART, CURLOPTTYPE_LONG, 233),

        
        CURLOPT(CURLOPT_PATH_AS_IS, CURLOPTTYPE_LONG, 234),

        
        CURLOPT(CURLOPT_PROXY_SERVICE_NAME, CURLOPTTYPE_STRINGPOINT, 235),

        
        CURLOPT(CURLOPT_SERVICE_NAME, CURLOPTTYPE_STRINGPOINT, 236),

        
        CURLOPT(CURLOPT_PIPEWAIT, CURLOPTTYPE_LONG, 237),

        
        CURLOPT(CURLOPT_DEFAULT_PROTOCOL, CURLOPTTYPE_STRINGPOINT, 238),

        
        CURLOPT(CURLOPT_STREAM_WEIGHT, CURLOPTTYPE_LONG, 239),

        
        CURLOPT(CURLOPT_STREAM_DEPENDS, CURLOPTTYPE_OBJECTPOINT, 240),

        
        CURLOPT(CURLOPT_STREAM_DEPENDS_E, CURLOPTTYPE_OBJECTPOINT, 241),

        
        CURLOPT(CURLOPT_TFTP_NO_OPTIONS, CURLOPTTYPE_LONG, 242),

        

        CURLOPT(CURLOPT_CONNECT_TO, CURLOPTTYPE_SLISTPOINT, 243),

        
        CURLOPT(CURLOPT_TCP_FASTOPEN, CURLOPTTYPE_LONG, 244),

        

        CURLOPT(CURLOPT_KEEP_SENDING_ON_ERROR, CURLOPTTYPE_LONG, 245),

        

        CURLOPT(CURLOPT_PROXY_CAINFO, CURLOPTTYPE_STRINGPOINT, 246),

        

        CURLOPT(CURLOPT_PROXY_CAPATH, CURLOPTTYPE_STRINGPOINT, 247),

        

        CURLOPT(CURLOPT_PROXY_SSL_VERIFYPEER, CURLOPTTYPE_LONG, 248),

        

        CURLOPT(CURLOPT_PROXY_SSL_VERIFYHOST, CURLOPTTYPE_LONG, 249),

        

        CURLOPT(CURLOPT_PROXY_SSLVERSION, CURLOPTTYPE_VALUES, 250),

        
        CURLOPT(CURLOPT_PROXY_TLSAUTH_USERNAME, CURLOPTTYPE_STRINGPOINT, 251),

        
        CURLOPT(CURLOPT_PROXY_TLSAUTH_PASSWORD, CURLOPTTYPE_STRINGPOINT, 252),

        
        CURLOPT(CURLOPT_PROXY_TLSAUTH_TYPE, CURLOPTTYPE_STRINGPOINT, 253),

        
        CURLOPT(CURLOPT_PROXY_SSLCERT, CURLOPTTYPE_STRINGPOINT, 254),

        

        CURLOPT(CURLOPT_PROXY_SSLCERTTYPE, CURLOPTTYPE_STRINGPOINT, 255),

        
        CURLOPT(CURLOPT_PROXY_SSLKEY, CURLOPTTYPE_STRINGPOINT, 256),

        

        CURLOPT(CURLOPT_PROXY_SSLKEYTYPE, CURLOPTTYPE_STRINGPOINT, 257),

        
        CURLOPT(CURLOPT_PROXY_KEYPASSWD, CURLOPTTYPE_STRINGPOINT, 258),

        
        CURLOPT(CURLOPT_PROXY_SSL_CIPHER_LIST, CURLOPTTYPE_STRINGPOINT, 259),

        
        CURLOPT(CURLOPT_PROXY_CRLFILE, CURLOPTTYPE_STRINGPOINT, 260),

        

        CURLOPT(CURLOPT_PROXY_SSL_OPTIONS, CURLOPTTYPE_LONG, 261),

        
        CURLOPT(CURLOPT_PRE_PROXY, CURLOPTTYPE_STRINGPOINT, 262),

        

        CURLOPT(CURLOPT_PROXY_PINNEDPUBLICKEY, CURLOPTTYPE_STRINGPOINT, 263),

        
        CURLOPT(CURLOPT_ABSTRACT_UNIX_SOCKET, CURLOPTTYPE_STRINGPOINT, 264),

        
        CURLOPT(CURLOPT_SUPPRESS_CONNECT_HEADERS, CURLOPTTYPE_LONG, 265),

        
        CURLOPT(CURLOPT_REQUEST_TARGET, CURLOPTTYPE_STRINGPOINT, 266),

        
        CURLOPT(CURLOPT_SOCKS5_AUTH, CURLOPTTYPE_LONG, 267),

        
        CURLOPT(CURLOPT_SSH_COMPRESSION, CURLOPTTYPE_LONG, 268),

        
        CURLOPT(CURLOPT_MIMEPOST, CURLOPTTYPE_OBJECTPOINT, 269),

        

        CURLOPT(CURLOPT_TIMEVALUE_LARGE, CURLOPTTYPE_OFF_T, 270),

        
        CURLOPT(CURLOPT_HAPPY_EYEBALLS_TIMEOUT_MS, CURLOPTTYPE_LONG, 271),

        
        CURLOPT(CURLOPT_RESOLVER_START_FUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 272),

        
        CURLOPT(CURLOPT_RESOLVER_START_DATA, CURLOPTTYPE_CBPOINT, 273),

        
        CURLOPT(CURLOPT_HAPROXYPROTOCOL, CURLOPTTYPE_LONG, 274),

        
        CURLOPT(CURLOPT_DNS_SHUFFLE_ADDRESSES, CURLOPTTYPE_LONG, 275),

        
        CURLOPT(CURLOPT_TLS13_CIPHERS, CURLOPTTYPE_STRINGPOINT, 276),
        CURLOPT(CURLOPT_PROXY_TLS13_CIPHERS, CURLOPTTYPE_STRINGPOINT, 277),

        
        CURLOPT(CURLOPT_DISALLOW_USERNAME_IN_URL, CURLOPTTYPE_LONG, 278),

        
        CURLOPT(CURLOPT_DOH_URL, CURLOPTTYPE_STRINGPOINT, 279),

        
        CURLOPT(CURLOPT_UPLOAD_BUFFERSIZE, CURLOPTTYPE_LONG, 280),

        
        CURLOPT(CURLOPT_UPKEEP_INTERVAL_MS, CURLOPTTYPE_LONG, 281),

        
        CURLOPT(CURLOPT_CURLU, CURLOPTTYPE_OBJECTPOINT, 282),

        
        CURLOPT(CURLOPT_TRAILERFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 283),

        
        CURLOPT(CURLOPT_TRAILERDATA, CURLOPTTYPE_CBPOINT, 284),

        
        CURLOPT(CURLOPT_HTTP09_ALLOWED, CURLOPTTYPE_LONG, 285),

        
        CURLOPT(CURLOPT_ALTSVC_CTRL, CURLOPTTYPE_LONG, 286),

        
        CURLOPT(CURLOPT_ALTSVC, CURLOPTTYPE_STRINGPOINT, 287),

        

        CURLOPT(CURLOPT_MAXAGE_CONN, CURLOPTTYPE_LONG, 288),

        
        CURLOPT(CURLOPT_SASL_AUTHZID, CURLOPTTYPE_STRINGPOINT, 289),

        
        CURLOPT(CURLOPT_MAIL_RCPT_ALLLOWFAILS, CURLOPTTYPE_LONG, 290),

        
        CURLOPT(CURLOPT_SSLCERT_BLOB, CURLOPTTYPE_BLOB, 291),
        CURLOPT(CURLOPT_SSLKEY_BLOB, CURLOPTTYPE_BLOB, 292),
        CURLOPT(CURLOPT_PROXY_SSLCERT_BLOB, CURLOPTTYPE_BLOB, 293),
        CURLOPT(CURLOPT_PROXY_SSLKEY_BLOB, CURLOPTTYPE_BLOB, 294),
        CURLOPT(CURLOPT_ISSUERCERT_BLOB, CURLOPTTYPE_BLOB, 295),

        
        CURLOPT(CURLOPT_PROXY_ISSUERCERT, CURLOPTTYPE_STRINGPOINT, 296),
        CURLOPT(CURLOPT_PROXY_ISSUERCERT_BLOB, CURLOPTTYPE_BLOB, 297),

        

        CURLOPT(CURLOPT_SSL_EC_CURVES, CURLOPTTYPE_STRINGPOINT, 298),

        
        CURLOPT(CURLOPT_HSTS_CTRL, CURLOPTTYPE_LONG, 299),
        
        CURLOPT(CURLOPT_HSTS, CURLOPTTYPE_STRINGPOINT, 300),

        
        CURLOPT(CURLOPT_HSTSREADFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 301),
        CURLOPT(CURLOPT_HSTSREADDATA, CURLOPTTYPE_CBPOINT, 302),

        
        CURLOPT(CURLOPT_HSTSWRITEFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 303),
        CURLOPT(CURLOPT_HSTSWRITEDATA, CURLOPTTYPE_CBPOINT, 304),

        
        CURLOPT(CURLOPT_AWS_SIGV4, CURLOPTTYPE_STRINGPOINT, 305),

        
        CURLOPT(CURLOPT_DOH_SSL_VERIFYPEER, CURLOPTTYPE_LONG, 306),

        
        CURLOPT(CURLOPT_DOH_SSL_VERIFYHOST, CURLOPTTYPE_LONG, 307),

        
        CURLOPT(CURLOPT_DOH_SSL_VERIFYSTATUS, CURLOPTTYPE_LONG, 308),

        

        CURLOPT(CURLOPT_CAINFO_BLOB, CURLOPTTYPE_BLOB, 309),

        

        CURLOPT(CURLOPT_PROXY_CAINFO_BLOB, CURLOPTTYPE_BLOB, 310),

        
        CURLOPT(CURLOPT_SSH_HOST_PUBLIC_KEY_SHA256, CURLOPTTYPE_STRINGPOINT, 311),

        

        CURLOPT(CURLOPT_PREREQFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 312),

        
        CURLOPT(CURLOPT_PREREQDATA, CURLOPTTYPE_CBPOINT, 313),

        

        CURLOPT(CURLOPT_MAXLIFETIME_CONN, CURLOPTTYPE_LONG, 314),

        
        CURLOPT(CURLOPT_MIME_OPTIONS, CURLOPTTYPE_LONG, 315),

        

        CURLOPT(CURLOPT_SSH_HOSTKEYFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 316),

        
        CURLOPT(CURLOPT_SSH_HOSTKEYDATA, CURLOPTTYPE_CBPOINT, 317),

        

        CURLOPT(CURLOPT_PROTOCOLS_STR, CURLOPTTYPE_STRINGPOINT, 318),

        
        CURLOPT(CURLOPT_REDIR_PROTOCOLS_STR, CURLOPTTYPE_STRINGPOINT, 319),

        CURLOPT_LASTENTRY 
    } CURLoption;

#ifndef CURL_NO_OLDIES 

#define CURLOPT_POST301 CURLOPT_POSTREDIR

#define CURLOPT_SSLKEYPASSWD CURLOPT_KEYPASSWD
#define CURLOPT_FTPAPPEND CURLOPT_APPEND
#define CURLOPT_FTPLISTONLY CURLOPT_DIRLISTONLY
#define CURLOPT_FTP_SSL CURLOPT_USE_SSL

    

#define CURLOPT_SSLCERTPASSWD CURLOPT_KEYPASSWD
#define CURLOPT_KRB4LEVEL CURLOPT_KRBLEVEL

#define CURLOPT_FTP_RESPONSE_TIMEOUT CURLOPT_SERVER_RESPONSE_TIMEOUT

#else

#undef CURLOPT_DNS_USE_GLOBAL_CACHE 
#endif

    

#define CURL_IPRESOLVE_WHATEVER                                                                                                                                                                                       \
    0                       

#define CURL_IPRESOLVE_V4 1 
#define CURL_IPRESOLVE_V6 2 

    
#define CURLOPT_RTSPHEADER CURLOPT_HTTPHEADER

    
    enum
    {
        CURL_HTTP_VERSION_NONE,              

        CURL_HTTP_VERSION_1_0,               
        CURL_HTTP_VERSION_1_1,               
        CURL_HTTP_VERSION_2_0,               
        CURL_HTTP_VERSION_2TLS,              
        CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE, 

        CURL_HTTP_VERSION_3 = 30,            

        CURL_HTTP_VERSION_LAST               
    };

#define CURL_HTTP_VERSION_2 CURL_HTTP_VERSION_2_0

    

    enum
    {
        CURL_RTSPREQ_NONE, 
        CURL_RTSPREQ_OPTIONS,
        CURL_RTSPREQ_DESCRIBE,
        CURL_RTSPREQ_ANNOUNCE,
        CURL_RTSPREQ_SETUP,
        CURL_RTSPREQ_PLAY,
        CURL_RTSPREQ_PAUSE,
        CURL_RTSPREQ_TEARDOWN,
        CURL_RTSPREQ_GET_PARAMETER,
        CURL_RTSPREQ_SET_PARAMETER,
        CURL_RTSPREQ_RECORD,
        CURL_RTSPREQ_RECEIVE,
        CURL_RTSPREQ_LAST 
    };

    
    enum CURL_NETRC_OPTION
    {
        CURL_NETRC_IGNORED,  

        CURL_NETRC_OPTIONAL, 

        CURL_NETRC_REQUIRED, 

        CURL_NETRC_LAST
    };

    enum
    {
        CURL_SSLVERSION_DEFAULT,
        CURL_SSLVERSION_TLSv1, 
        CURL_SSLVERSION_SSLv2,
        CURL_SSLVERSION_SSLv3,
        CURL_SSLVERSION_TLSv1_0,
        CURL_SSLVERSION_TLSv1_1,
        CURL_SSLVERSION_TLSv1_2,
        CURL_SSLVERSION_TLSv1_3,

        CURL_SSLVERSION_LAST 
    };

    enum
    {
        CURL_SSLVERSION_MAX_NONE = 0,
        CURL_SSLVERSION_MAX_DEFAULT = (CURL_SSLVERSION_TLSv1 << 16),
        CURL_SSLVERSION_MAX_TLSv1_0 = (CURL_SSLVERSION_TLSv1_0 << 16),
        CURL_SSLVERSION_MAX_TLSv1_1 = (CURL_SSLVERSION_TLSv1_1 << 16),
        CURL_SSLVERSION_MAX_TLSv1_2 = (CURL_SSLVERSION_TLSv1_2 << 16),
        CURL_SSLVERSION_MAX_TLSv1_3 = (CURL_SSLVERSION_TLSv1_3 << 16),

        
        CURL_SSLVERSION_MAX_LAST = (CURL_SSLVERSION_LAST << 16)
    };

    enum CURL_TLSAUTH
    {
        CURL_TLSAUTH_NONE,
        CURL_TLSAUTH_SRP,
        CURL_TLSAUTH_LAST 
    };

    

#define CURL_REDIR_GET_ALL 0
#define CURL_REDIR_POST_301 1
#define CURL_REDIR_POST_302 2
#define CURL_REDIR_POST_303 4
#define CURL_REDIR_POST_ALL (CURL_REDIR_POST_301 | CURL_REDIR_POST_302 | CURL_REDIR_POST_303)

    typedef enum
    {
        CURL_TIMECOND_NONE,

        CURL_TIMECOND_IFMODSINCE,
        CURL_TIMECOND_IFUNMODSINCE,
        CURL_TIMECOND_LASTMOD,

        CURL_TIMECOND_LAST
    } curl_TimeCond;

#define CURL_ZERO_TERMINATED ((size_t)-1)

    

    CURL_EXTERN int curl_strequal(const char* s1, const char* s2);
    CURL_EXTERN int curl_strnequal(const char* s1, const char* s2, size_t n);

    
    typedef struct curl_mime curl_mime;         
    typedef struct curl_mimepart curl_mimepart; 

#define CURLMIMEOPT_FORMESCAPE (1 << 0) 

    

    CURL_EXTERN curl_mime* curl_mime_init(CURL* easy);

    

    CURL_EXTERN void curl_mime_free(curl_mime* mime);

    

    CURL_EXTERN curl_mimepart* curl_mime_addpart(curl_mime* mime);

    

    CURL_EXTERN CURLcode curl_mime_name(curl_mimepart* part, const char* name);

    

    CURL_EXTERN CURLcode curl_mime_filename(curl_mimepart* part, const char* filename);

    

    CURL_EXTERN CURLcode curl_mime_type(curl_mimepart* part, const char* mimetype);

    

    CURL_EXTERN CURLcode curl_mime_encoder(curl_mimepart* part, const char* encoding);

    

    CURL_EXTERN CURLcode curl_mime_data(curl_mimepart* part, const char* data, size_t datasize);

    

    CURL_EXTERN CURLcode curl_mime_filedata(curl_mimepart* part, const char* filename);

    

    CURL_EXTERN CURLcode curl_mime_data_cb(curl_mimepart* part, curl_off_t datasize, curl_read_callback readfunc, curl_seek_callback seekfunc, curl_free_callback freefunc, void* arg);

    

    CURL_EXTERN CURLcode curl_mime_subparts(curl_mimepart* part, curl_mime* subparts);
    

    CURL_EXTERN CURLcode curl_mime_headers(curl_mimepart* part, struct curl_slist* headers, int take_ownership);

    typedef enum
    {
        CURLFORM_NOTHING, 
        CURLFORM_COPYNAME,
        CURLFORM_PTRNAME,
        CURLFORM_NAMELENGTH,
        CURLFORM_COPYCONTENTS,
        CURLFORM_PTRCONTENTS,
        CURLFORM_CONTENTSLENGTH,
        CURLFORM_FILECONTENT,
        CURLFORM_ARRAY,
        CURLFORM_OBSOLETE,
        CURLFORM_FILE,

        CURLFORM_BUFFER,
        CURLFORM_BUFFERPTR,
        CURLFORM_BUFFERLENGTH,

        CURLFORM_CONTENTTYPE,
        CURLFORM_CONTENTHEADER,
        CURLFORM_FILENAME,
        CURLFORM_END,
        CURLFORM_OBSOLETE2,

        CURLFORM_STREAM,
        CURLFORM_CONTENTLEN, 

        CURLFORM_LASTENTRY 
    } CURLformoption;

    
    struct curl_forms
    {
        CURLformoption option;
        const char* value;
    };

    
    

    typedef enum
    {
        CURL_FORMADD_OK, 

        CURL_FORMADD_MEMORY,
        CURL_FORMADD_OPTION_TWICE,
        CURL_FORMADD_NULL,
        CURL_FORMADD_UNKNOWN_OPTION,
        CURL_FORMADD_INCOMPLETE,
        CURL_FORMADD_ILLEGAL_ARRAY,
        CURL_FORMADD_DISABLED, 

        CURL_FORMADD_LAST 
    } CURLFORMcode;

    

    CURL_EXTERN CURLFORMcode curl_formadd(struct curl_httppost** httppost, struct curl_httppost** last_post, ...);

    

    typedef size_t (*curl_formget_callback)(void* arg, const char* buf, size_t len);

    

    CURL_EXTERN int curl_formget(struct curl_httppost* form, void* arg, curl_formget_callback append);
    

    CURL_EXTERN void curl_formfree(struct curl_httppost* form);

    

    CURL_EXTERN char* curl_getenv(const char* variable);

    

    CURL_EXTERN char* curl_version(void);

    

    CURL_EXTERN char* curl_easy_escape(CURL* handle, const char* string, int length);

    
    CURL_EXTERN char* curl_escape(const char* string, int length);

    

    CURL_EXTERN char* curl_easy_unescape(CURL* handle, const char* string, int length, int* outlength);

    
    CURL_EXTERN char* curl_unescape(const char* string, int length);

    

    CURL_EXTERN void curl_free(void* p);

    

    CURL_EXTERN CURLcode curl_global_init(long flags);

    

    CURL_EXTERN CURLcode curl_global_init_mem(long flags, curl_malloc_callback m, curl_free_callback f, curl_realloc_callback r, curl_strdup_callback s, curl_calloc_callback c);

    

    CURL_EXTERN void curl_global_cleanup(void);

    
    struct curl_slist
    {
        char* data;
        struct curl_slist* next;
    };

    

    struct curl_ssl_backend
    {
        curl_sslbackend id;
        const char* name;
    };
    typedef struct curl_ssl_backend curl_ssl_backend;

    typedef enum
    {
        CURLSSLSET_OK = 0,
        CURLSSLSET_UNKNOWN_BACKEND,
        CURLSSLSET_TOO_LATE,
        CURLSSLSET_NO_BACKENDS 
    } CURLsslset;

    CURL_EXTERN CURLsslset curl_global_sslset(curl_sslbackend id, const char* name, const curl_ssl_backend*** avail);

    

    CURL_EXTERN struct curl_slist* curl_slist_append(struct curl_slist*, const char*);

    

    CURL_EXTERN void curl_slist_free_all(struct curl_slist*);

    

    CURL_EXTERN time_t curl_getdate(const char* p, const time_t* unused);

    

    struct curl_certinfo
    {
        int num_of_certs;             
        struct curl_slist** certinfo; 

    };

    

    struct curl_tlssessioninfo
    {
        curl_sslbackend backend;
        void* internals;
    };

#define CURLINFO_STRING 0x100000
#define CURLINFO_LONG 0x200000
#define CURLINFO_DOUBLE 0x300000
#define CURLINFO_SLIST 0x400000
#define CURLINFO_PTR 0x400000 
#define CURLINFO_SOCKET 0x500000
#define CURLINFO_OFF_T 0x600000
#define CURLINFO_MASK 0x0fffff
#define CURLINFO_TYPEMASK 0xf00000

    typedef enum
    {
        CURLINFO_NONE, 
        CURLINFO_EFFECTIVE_URL = CURLINFO_STRING + 1,
        CURLINFO_RESPONSE_CODE = CURLINFO_LONG + 2,
        CURLINFO_TOTAL_TIME = CURLINFO_DOUBLE + 3,
        CURLINFO_NAMELOOKUP_TIME = CURLINFO_DOUBLE + 4,
        CURLINFO_CONNECT_TIME = CURLINFO_DOUBLE + 5,
        CURLINFO_PRETRANSFER_TIME = CURLINFO_DOUBLE + 6,
        CURLINFO_SIZE_UPLOAD = CURLINFO_DOUBLE + 7,
        CURLINFO_SIZE_UPLOAD_T = CURLINFO_OFF_T + 7,
        CURLINFO_SIZE_DOWNLOAD = CURLINFO_DOUBLE + 8,
        CURLINFO_SIZE_DOWNLOAD_T = CURLINFO_OFF_T + 8,
        CURLINFO_SPEED_DOWNLOAD = CURLINFO_DOUBLE + 9,
        CURLINFO_SPEED_DOWNLOAD_T = CURLINFO_OFF_T + 9,
        CURLINFO_SPEED_UPLOAD = CURLINFO_DOUBLE + 10,
        CURLINFO_SPEED_UPLOAD_T = CURLINFO_OFF_T + 10,
        CURLINFO_HEADER_SIZE = CURLINFO_LONG + 11,
        CURLINFO_REQUEST_SIZE = CURLINFO_LONG + 12,
        CURLINFO_SSL_VERIFYRESULT = CURLINFO_LONG + 13,
        CURLINFO_FILETIME = CURLINFO_LONG + 14,
        CURLINFO_FILETIME_T = CURLINFO_OFF_T + 14,
        CURLINFO_CONTENT_LENGTH_DOWNLOAD = CURLINFO_DOUBLE + 15,
        CURLINFO_CONTENT_LENGTH_DOWNLOAD_T = CURLINFO_OFF_T + 15,
        CURLINFO_CONTENT_LENGTH_UPLOAD = CURLINFO_DOUBLE + 16,
        CURLINFO_CONTENT_LENGTH_UPLOAD_T = CURLINFO_OFF_T + 16,
        CURLINFO_STARTTRANSFER_TIME = CURLINFO_DOUBLE + 17,
        CURLINFO_CONTENT_TYPE = CURLINFO_STRING + 18,
        CURLINFO_REDIRECT_TIME = CURLINFO_DOUBLE + 19,
        CURLINFO_REDIRECT_COUNT = CURLINFO_LONG + 20,
        CURLINFO_PRIVATE = CURLINFO_STRING + 21,
        CURLINFO_HTTP_CONNECTCODE = CURLINFO_LONG + 22,
        CURLINFO_HTTPAUTH_AVAIL = CURLINFO_LONG + 23,
        CURLINFO_PROXYAUTH_AVAIL = CURLINFO_LONG + 24,
        CURLINFO_OS_ERRNO = CURLINFO_LONG + 25,
        CURLINFO_NUM_CONNECTS = CURLINFO_LONG + 26,
        CURLINFO_SSL_ENGINES = CURLINFO_SLIST + 27,
        CURLINFO_COOKIELIST = CURLINFO_SLIST + 28,
        CURLINFO_LASTSOCKET = CURLINFO_LONG + 29,
        CURLINFO_FTP_ENTRY_PATH = CURLINFO_STRING + 30,
        CURLINFO_REDIRECT_URL = CURLINFO_STRING + 31,
        CURLINFO_PRIMARY_IP = CURLINFO_STRING + 32,
        CURLINFO_APPCONNECT_TIME = CURLINFO_DOUBLE + 33,
        CURLINFO_CERTINFO = CURLINFO_PTR + 34,
        CURLINFO_CONDITION_UNMET = CURLINFO_LONG + 35,
        CURLINFO_RTSP_SESSION_ID = CURLINFO_STRING + 36,
        CURLINFO_RTSP_CLIENT_CSEQ = CURLINFO_LONG + 37,
        CURLINFO_RTSP_SERVER_CSEQ = CURLINFO_LONG + 38,
        CURLINFO_RTSP_CSEQ_RECV = CURLINFO_LONG + 39,
        CURLINFO_PRIMARY_PORT = CURLINFO_LONG + 40,
        CURLINFO_LOCAL_IP = CURLINFO_STRING + 41,
        CURLINFO_LOCAL_PORT = CURLINFO_LONG + 42,
        CURLINFO_TLS_SESSION = CURLINFO_PTR + 43,
        CURLINFO_ACTIVESOCKET = CURLINFO_SOCKET + 44,
        CURLINFO_TLS_SSL_PTR = CURLINFO_PTR + 45,
        CURLINFO_HTTP_VERSION = CURLINFO_LONG + 46,
        CURLINFO_PROXY_SSL_VERIFYRESULT = CURLINFO_LONG + 47,
        CURLINFO_PROTOCOL = CURLINFO_LONG + 48,
        CURLINFO_SCHEME = CURLINFO_STRING + 49,
        CURLINFO_TOTAL_TIME_T = CURLINFO_OFF_T + 50,
        CURLINFO_NAMELOOKUP_TIME_T = CURLINFO_OFF_T + 51,
        CURLINFO_CONNECT_TIME_T = CURLINFO_OFF_T + 52,
        CURLINFO_PRETRANSFER_TIME_T = CURLINFO_OFF_T + 53,
        CURLINFO_STARTTRANSFER_TIME_T = CURLINFO_OFF_T + 54,
        CURLINFO_REDIRECT_TIME_T = CURLINFO_OFF_T + 55,
        CURLINFO_APPCONNECT_TIME_T = CURLINFO_OFF_T + 56,
        CURLINFO_RETRY_AFTER = CURLINFO_OFF_T + 57,
        CURLINFO_EFFECTIVE_METHOD = CURLINFO_STRING + 58,
        CURLINFO_PROXY_ERROR = CURLINFO_LONG + 59,
        CURLINFO_REFERER = CURLINFO_STRING + 60,
        CURLINFO_CAINFO = CURLINFO_STRING + 61,
        CURLINFO_CAPATH = CURLINFO_STRING + 62,
        CURLINFO_LASTONE = 62
    } CURLINFO;

#define CURLINFO_HTTP_CODE CURLINFO_RESPONSE_CODE

    typedef enum
    {
        CURLCLOSEPOLICY_NONE, 

        CURLCLOSEPOLICY_OLDEST,
        CURLCLOSEPOLICY_LEAST_RECENTLY_USED,
        CURLCLOSEPOLICY_LEAST_TRAFFIC,
        CURLCLOSEPOLICY_SLOWEST,
        CURLCLOSEPOLICY_CALLBACK,

        CURLCLOSEPOLICY_LAST 
    } curl_closepolicy;

#define CURL_GLOBAL_SSL (1 << 0) 
#define CURL_GLOBAL_WIN32 (1 << 1)
#define CURL_GLOBAL_ALL (CURL_GLOBAL_SSL | CURL_GLOBAL_WIN32)
#define CURL_GLOBAL_NOTHING 0
#define CURL_GLOBAL_DEFAULT CURL_GLOBAL_ALL
#define CURL_GLOBAL_ACK_EINTR (1 << 2)

    

    
    typedef enum
    {
        CURL_LOCK_DATA_NONE = 0,
        

        CURL_LOCK_DATA_SHARE,
        CURL_LOCK_DATA_COOKIE,
        CURL_LOCK_DATA_DNS,
        CURL_LOCK_DATA_SSL_SESSION,
        CURL_LOCK_DATA_CONNECT,
        CURL_LOCK_DATA_PSL,
        CURL_LOCK_DATA_LAST
    } curl_lock_data;

    
    typedef enum
    {
        CURL_LOCK_ACCESS_NONE = 0,   
        CURL_LOCK_ACCESS_SHARED = 1, 
        CURL_LOCK_ACCESS_SINGLE = 2, 
        CURL_LOCK_ACCESS_LAST        
    } curl_lock_access;

    typedef void (*curl_lock_function)(CURL* handle, curl_lock_data data, curl_lock_access locktype, void* userptr);
    typedef void (*curl_unlock_function)(CURL* handle, curl_lock_data data, void* userptr);

    typedef enum
    {
        CURLSHE_OK,           
        CURLSHE_BAD_OPTION,   
        CURLSHE_IN_USE,       
        CURLSHE_INVALID,      
        CURLSHE_NOMEM,        
        CURLSHE_NOT_BUILT_IN, 
        CURLSHE_LAST          
    } CURLSHcode;

    typedef enum
    {
        CURLSHOPT_NONE,       
        CURLSHOPT_SHARE,      
        CURLSHOPT_UNSHARE,    
        CURLSHOPT_LOCKFUNC,   
        CURLSHOPT_UNLOCKFUNC, 
        CURLSHOPT_USERDATA,   

        CURLSHOPT_LAST        
    } CURLSHoption;

    CURL_EXTERN CURLSH* curl_share_init(void);
    CURL_EXTERN CURLSHcode curl_share_setopt(CURLSH*, CURLSHoption option, ...);
    CURL_EXTERN CURLSHcode curl_share_cleanup(CURLSH*);

    

    typedef enum
    {
        CURLVERSION_FIRST,
        CURLVERSION_SECOND,
        CURLVERSION_THIRD,
        CURLVERSION_FOURTH,
        CURLVERSION_FIFTH,
        CURLVERSION_SIXTH,
        CURLVERSION_SEVENTH,
        CURLVERSION_EIGHTH,
        CURLVERSION_NINTH,
        CURLVERSION_TENTH,
        CURLVERSION_LAST 
    } CURLversion;

#define CURLVERSION_NOW CURLVERSION_TENTH

    struct curl_version_info_data
    {
        CURLversion age;          
        const char* version;      
        unsigned int version_num; 
        const char* host;         
        int features;             
        const char* ssl_version;  
        long ssl_version_num;     
        const char* libz_version; 
        
        const char* const* protocols;

        
        const char* ares;
        int ares_num;

        
        const char* libidn;

        

        
        int iconv_ver_num;

        const char* libssh_version; 

        
        unsigned int brotli_ver_num; 

        const char* brotli_version;  

        
        unsigned int nghttp2_ver_num; 

        const char* nghttp2_version;  
        const char* quic_version;     

        
        const char* cainfo; 

        const char* capath; 

        
        unsigned int zstd_ver_num; 

        const char* zstd_version;  

        
        const char* hyper_version; 

        
        const char* gsasl_version; 
    };
    typedef struct curl_version_info_data curl_version_info_data;

#define CURL_VERSION_IPV6 (1 << 0) 
#define CURL_VERSION_KERBEROS4                                                                                                                                                                                        \
    (1 << 1)                       

#define CURL_VERSION_SSL (1 << 2)  
#define CURL_VERSION_LIBZ (1 << 3) 
#define CURL_VERSION_NTLM (1 << 4) 
#define CURL_VERSION_GSSNEGOTIATE                                                                                                                                                                                     \
    (1 << 5)                            

#define CURL_VERSION_DEBUG (1 << 6)     
#define CURL_VERSION_ASYNCHDNS (1 << 7) 
#define CURL_VERSION_SPNEGO (1 << 8)    
#define CURL_VERSION_LARGEFILE (1 << 9) 
#define CURL_VERSION_IDN                                                                                                                                                                                              \
    (1 << 10)                              

#define CURL_VERSION_SSPI (1 << 11)        
#define CURL_VERSION_CONV (1 << 12)        
#define CURL_VERSION_CURLDEBUG (1 << 13)   
#define CURL_VERSION_TLSAUTH_SRP (1 << 14) 
#define CURL_VERSION_NTLM_WB                                                                                                                                                                                          \
    (1 << 15)                               

#define CURL_VERSION_HTTP2 (1 << 16)        
#define CURL_VERSION_GSSAPI (1 << 17)       
#define CURL_VERSION_KERBEROS5 (1 << 18)    
#define CURL_VERSION_UNIX_SOCKETS (1 << 19) 
#define CURL_VERSION_PSL                                                                                                                                                                                              \
    (1 << 20)                              

#define CURL_VERSION_HTTPS_PROXY (1 << 21) 
#define CURL_VERSION_MULTI_SSL (1 << 22)   
#define CURL_VERSION_BROTLI (1 << 23)      
#define CURL_VERSION_ALTSVC (1 << 24)      
#define CURL_VERSION_HTTP3 (1 << 25)       
#define CURL_VERSION_ZSTD (1 << 26)        
#define CURL_VERSION_UNICODE (1 << 27)     
#define CURL_VERSION_HSTS (1 << 28)        
#define CURL_VERSION_GSASL (1 << 29)       
#define CURL_VERSION_THREADSAFE (1 << 30)  

    

    CURL_EXTERN curl_version_info_data* curl_version_info(CURLversion);

    

    CURL_EXTERN const char* curl_easy_strerror(CURLcode);

    

    CURL_EXTERN const char* curl_share_strerror(CURLSHcode);

    

    CURL_EXTERN CURLcode curl_easy_pause(CURL* handle, int bitmask);

#define CURLPAUSE_RECV (1 << 0)
#define CURLPAUSE_RECV_CONT (0)

#define CURLPAUSE_SEND (1 << 2)
#define CURLPAUSE_SEND_CONT (0)

#define CURLPAUSE_ALL (CURLPAUSE_RECV | CURLPAUSE_SEND)
#define CURLPAUSE_CONT (CURLPAUSE_RECV_CONT | CURLPAUSE_SEND_CONT)

#ifdef __cplusplus
}
#endif

#include "easy.h" 
#include "multi.h"
#include "urlapi.h"
#include "options.h"
#include "header.h"

#if defined(__GNUC__) && defined(__GNUC_MINOR__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)) && !defined(__cplusplus) && !defined(CURL_DISABLE_TYPECHECK)
#include "typecheck-gcc.h"
#else
#if defined(__STDC__) && (__STDC__ >= 1)

#define curl_easy_setopt(handle, opt, param) curl_easy_setopt(handle, opt, param)
#define curl_easy_getinfo(handle, info, arg) curl_easy_getinfo(handle, info, arg)
#define curl_share_setopt(share, opt, param) curl_share_setopt(share, opt, param)
#define curl_multi_setopt(handle, opt, param) curl_multi_setopt(handle, opt, param)
#endif 
#endif 

#endif 
