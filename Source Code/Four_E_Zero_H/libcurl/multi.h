#ifndef CURLINC_MULTI_H
#define CURLINC_MULTI_H

#include "curl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(BUILDING_LIBCURL) || defined(CURL_STRICTER)
    typedef struct Curl_multi CURLM;
#else
typedef void CURLM;
#endif

    typedef enum
    {
        CURLM_CALL_MULTI_PERFORM = -1, 

        CURLM_OK,
        CURLM_BAD_HANDLE,            
        CURLM_BAD_EASY_HANDLE,       
        CURLM_OUT_OF_MEMORY,         
        CURLM_INTERNAL_ERROR,        
        CURLM_BAD_SOCKET,            
        CURLM_UNKNOWN_OPTION,        
        CURLM_ADDED_ALREADY,         

        CURLM_RECURSIVE_API_CALL,    

        CURLM_WAKEUP_FAILURE,        
        CURLM_BAD_FUNCTION_ARGUMENT, 
        CURLM_ABORTED_BY_CALLBACK,
        CURLM_UNRECOVERABLE_POLL,
        CURLM_LAST
    } CURLMcode;

#define CURLM_CALL_MULTI_SOCKET CURLM_CALL_MULTI_PERFORM

#define CURLPIPE_NOTHING 0L
#define CURLPIPE_HTTP1 1L
#define CURLPIPE_MULTIPLEX 2L

    typedef enum
    {
        CURLMSG_NONE, 
        CURLMSG_DONE, 

        CURLMSG_LAST  
    } CURLMSG;

    struct CURLMsg
    {
        CURLMSG msg;       
        CURL* easy_handle; 
        union
        {
            void* whatever;  
            CURLcode result; 
        } data;
    };
    typedef struct CURLMsg CURLMsg;

#define CURL_WAIT_POLLIN 0x0001
#define CURL_WAIT_POLLPRI 0x0002
#define CURL_WAIT_POLLOUT 0x0004

    struct curl_waitfd
    {
        curl_socket_t fd;
        short events;
        short revents; 
    };

    

    CURL_EXTERN CURLM* curl_multi_init(void);

    

    CURL_EXTERN CURLMcode curl_multi_add_handle(CURLM* multi_handle, CURL* curl_handle);

    

    CURL_EXTERN CURLMcode curl_multi_remove_handle(CURLM* multi_handle, CURL* curl_handle);

    

    CURL_EXTERN CURLMcode curl_multi_fdset(CURLM* multi_handle, fd_set* read_fd_set, fd_set* write_fd_set, fd_set* exc_fd_set, int* max_fd);

    

    CURL_EXTERN CURLMcode curl_multi_wait(CURLM* multi_handle, struct curl_waitfd extra_fds[], unsigned int extra_nfds, int timeout_ms, int* ret);

    

    CURL_EXTERN CURLMcode curl_multi_poll(CURLM* multi_handle, struct curl_waitfd extra_fds[], unsigned int extra_nfds, int timeout_ms, int* ret);

    

    CURL_EXTERN CURLMcode curl_multi_wakeup(CURLM* multi_handle);

    

    CURL_EXTERN CURLMcode curl_multi_perform(CURLM* multi_handle, int* running_handles);

    

    CURL_EXTERN CURLMcode curl_multi_cleanup(CURLM* multi_handle);

    

    CURL_EXTERN CURLMsg* curl_multi_info_read(CURLM* multi_handle, int* msgs_in_queue);

    

    CURL_EXTERN const char* curl_multi_strerror(CURLMcode);

#define CURL_POLL_NONE 0
#define CURL_POLL_IN 1
#define CURL_POLL_OUT 2
#define CURL_POLL_INOUT 3
#define CURL_POLL_REMOVE 4

#define CURL_SOCKET_TIMEOUT CURL_SOCKET_BAD

#define CURL_CSELECT_IN 0x01
#define CURL_CSELECT_OUT 0x02
#define CURL_CSELECT_ERR 0x04

    typedef int (*curl_socket_callback)(CURL* easy,      
                                        curl_socket_t s, 
                                        int what,        
                                        void* userp,     

                                        void* socketp);  

    

    typedef int (*curl_multi_timer_callback)(CURLM* multi,    
                                             long timeout_ms, 
                                             void* userp);    

    CURL_EXTERN CURLMcode curl_multi_socket(CURLM* multi_handle, curl_socket_t s, int* running_handles);

    CURL_EXTERN CURLMcode curl_multi_socket_action(CURLM* multi_handle, curl_socket_t s, int ev_bitmask, int* running_handles);

    CURL_EXTERN CURLMcode curl_multi_socket_all(CURLM* multi_handle, int* running_handles);

#ifndef CURL_ALLOW_OLD_MULTI_SOCKET

#define curl_multi_socket(x, y, z) curl_multi_socket_action(x, y, 0, z)
#endif

    

    CURL_EXTERN CURLMcode curl_multi_timeout(CURLM* multi_handle, long* milliseconds);

    typedef enum
    {
        
        CURLOPT(CURLMOPT_SOCKETFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 1),

        
        CURLOPT(CURLMOPT_SOCKETDATA, CURLOPTTYPE_OBJECTPOINT, 2),

        
        CURLOPT(CURLMOPT_PIPELINING, CURLOPTTYPE_LONG, 3),

        
        CURLOPT(CURLMOPT_TIMERFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 4),

        
        CURLOPT(CURLMOPT_TIMERDATA, CURLOPTTYPE_OBJECTPOINT, 5),

        
        CURLOPT(CURLMOPT_MAXCONNECTS, CURLOPTTYPE_LONG, 6),

        
        CURLOPT(CURLMOPT_MAX_HOST_CONNECTIONS, CURLOPTTYPE_LONG, 7),

        
        CURLOPT(CURLMOPT_MAX_PIPELINE_LENGTH, CURLOPTTYPE_LONG, 8),

        

        CURLOPT(CURLMOPT_CONTENT_LENGTH_PENALTY_SIZE, CURLOPTTYPE_OFF_T, 9),

        

        CURLOPT(CURLMOPT_CHUNK_LENGTH_PENALTY_SIZE, CURLOPTTYPE_OFF_T, 10),

        
        CURLOPT(CURLMOPT_PIPELINING_SITE_BL, CURLOPTTYPE_OBJECTPOINT, 11),

        
        CURLOPT(CURLMOPT_PIPELINING_SERVER_BL, CURLOPTTYPE_OBJECTPOINT, 12),

        
        CURLOPT(CURLMOPT_MAX_TOTAL_CONNECTIONS, CURLOPTTYPE_LONG, 13),

        
        CURLOPT(CURLMOPT_PUSHFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 14),

        
        CURLOPT(CURLMOPT_PUSHDATA, CURLOPTTYPE_OBJECTPOINT, 15),

        
        CURLOPT(CURLMOPT_MAX_CONCURRENT_STREAMS, CURLOPTTYPE_LONG, 16),

        CURLMOPT_LASTENTRY 
    } CURLMoption;

    

    CURL_EXTERN CURLMcode curl_multi_setopt(CURLM* multi_handle, CURLMoption option, ...);

    

    CURL_EXTERN CURLMcode curl_multi_assign(CURLM* multi_handle, curl_socket_t sockfd, void* sockp);

#define CURL_PUSH_OK 0
#define CURL_PUSH_DENY 1
#define CURL_PUSH_ERROROUT 2 

    struct curl_pushheaders; 

    CURL_EXTERN char* curl_pushheader_bynum(struct curl_pushheaders* h, size_t num);
    CURL_EXTERN char* curl_pushheader_byname(struct curl_pushheaders* h, const char* name);

    typedef int (*curl_push_callback)(CURL* parent, CURL* easy, size_t num_headers, struct curl_pushheaders* headers, void* userp);

#ifdef __cplusplus
} 
#endif

#endif
