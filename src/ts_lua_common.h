/*
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/


#ifndef _TS_LUA_COMMON_H
#define _TS_LUA_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tcl.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <ts/ts.h>
#include <ts/experimental.h>
#include <ts/remap.h>

#define TS_LUA_FUNCTION_REMAP                   "do_remap"
#define TS_LUA_FUNCTION_POST_REMAP              "do_post_remap"
#define TS_LUA_FUNCTION_CACHE_LOOKUP_COMPLETE   "do_cache_lookup_complete"
#define TS_LUA_FUNCTION_SEND_REQUEST            "do_send_request"
#define TS_LUA_FUNCTION_READ_RESPONSE           "do_read_response"
#define TS_LUA_FUNCTION_SEND_RESPONSE           "do_send_response"

#define TS_LUA_MAX_SCRIPT_FNAME_LENGTH      1024
#define TS_LUA_MAX_CONFIG_VARS_COUNT        256
#define TS_LUA_MAX_URL_LENGTH               2048
#define TS_LUA_MAX_OVEC_SIZE                (3 * 32)
#define TS_LUA_MAX_RESIDENT_PCRE            64

#define TS_LUA_MIN_ALIGN                    sizeof(void*)
#define TS_LUA_MEM_ALIGN(size)              (((size) + ((TS_LUA_MIN_ALIGN) - 1)) & ~((TS_LUA_MIN_ALIGN) - 1))
#define TS_LUA_ALIGN_COUNT(size)            (size / TS_LUA_MIN_ALIGN)

#define TS_LUA_DEBUG_TAG                    "ts_lua"


typedef struct {
    Tcl_HashTable   t;
    TSMutex         mutexp;
} ts_lua_hash_map;


typedef struct {
    char    *content;
    char    script[TS_LUA_MAX_SCRIPT_FNAME_LENGTH];
    void    *conf_vars[TS_LUA_MAX_CONFIG_VARS_COUNT];

    ts_lua_hash_map regex_map;

} ts_lua_instance_conf;


typedef struct {
    lua_State   *lua;
    TSMutex     mutexp;
    int         gref;
} ts_lua_main_ctx;


typedef struct {
    lua_State   *lua;
    TSHttpTxn   txnp;
    TSCont      main_contp;

    TSMBuffer   client_request_bufp;
    TSMLoc      client_request_hdrp;
    TSMLoc      client_request_url;

    TSMBuffer   server_request_bufp;
    TSMLoc      server_request_hdrp;
    TSMLoc      server_request_url;

    TSMBuffer   server_response_bufp;
    TSMLoc      server_response_hdrp;

    TSMBuffer   client_response_bufp;
    TSMLoc      client_response_hdrp;

    TSMBuffer   cached_response_bufp;
    TSMLoc      cached_response_hdrp;

    ts_lua_main_ctx   *mctx;
    TSRemapRequestInfo *rri;

    ts_lua_instance_conf *instance_conf;

    int         intercept_type;
    int         ref;

} ts_lua_http_ctx;


typedef struct {
    TSVIO               vio;
    TSIOBuffer          buffer;
    TSIOBufferReader    reader;
} ts_lua_io_handle;

typedef struct {
    TSVIO               output_vio;
    TSIOBuffer          output_buffer;
    TSIOBufferReader    output_reader;

    int64_t             total;
    ts_lua_http_ctx     *hctx;
    int                 eos;

} ts_lua_transform_ctx;


struct ict_item;
struct ict_ctx;
typedef int (*ict_clean)(struct ict_item *item);

typedef struct ict_item {
    struct ict_item *next;
    struct ict_ctx  *ictx;

    TSCont      contp;
    ict_clean   cleanup;
    void        *data;
    int         deleted:1;
} ts_lua_http_intercept_item;

typedef struct ict_ctx {
    lua_State           *lua;
    TSCont              contp;
    ts_lua_io_handle    input;
    ts_lua_io_handle    output;
    TSVConn             net_vc;

    ts_lua_http_ctx     *hctx;

    struct ict_item     *ict_chain;

    int64_t             to_flush;
    int                 ref;

    int                 recv_complete:1;
    int                 send_complete:1;
    int                 all_ready:1;
} ts_lua_http_intercept_ctx;

#define TS_LUA_RELEASE_IO_HANDLE(ih) do {   \
    if (ih->reader) {                       \
        TSIOBufferReaderFree(ih->reader);   \
        ih->reader = NULL;                  \
    }                                       \
    if (ih->buffer) {                       \
        TSIOBufferDestroy(ih->buffer);      \
        ih->buffer = NULL;                  \
    }                                       \
} while (0)

#define TS_LUA_ADD_INTERCEPT_ITEM(ictx, item, contp, func, d)   \
            {   item->cleanup = func;  \
                item->data = d;  \
                item->ictx = ictx;  \
                item->contp = contp;  \
                item->deleted = 0;  \
                item->next = ictx->ict_chain;   \
                ictx->ict_chain = item;}

#endif

