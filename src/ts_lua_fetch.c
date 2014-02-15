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


#include <arpa/inet.h>
#include <ts_fetcher/ts_fetcher.h>
#include "ts_lua_util.h"

typedef struct {
    TSIOBuffer          buffer;
    TSIOBufferReader    reader;
    TSAction            read_action;
    ts_lua_http_intercept_ctx   *ictx;

    int                 streaming:1;
    int                 suspended:1;
    int                 error:1;
    int                 header_complete:1;
    int                 body_complete:1;
} ts_lua_fetch_ctx;

static int ts_lua_fetch(lua_State *L);
static int ts_lua_fetch_read(lua_State *L);

static int ts_lua_fetch_handler(TSCont contp, TSEvent event, void *edata);
static int ts_lua_fetch_cleanup(struct ict_item *item);


void
ts_lua_inject_fetch_api(lua_State *L)
{
    lua_pushcfunction(L, ts_lua_fetch);
    lua_setfield(L, -2, "fetch");

    lua_pushcfunction(L, ts_lua_fetch_read);
    lua_setfield(L, -2, "fetch_read");
}

static int
ts_lua_fetch(lua_State *L)
{
    TSCont              contp;
    http_fetcher        *fch;
    const char          *url, *value, *key, *opt;
    size_t              len, val_len, key_len, opt_len;
    const char          *method, *body;
    size_t              method_len, body_len;
    int                 i, n, size;
    char                c;
    int                 flags, streaming;
    struct sockaddr     client_addr;
    ts_lua_fetch_ctx    *fctx;
    ts_lua_http_intercept_item  *item;
    ts_lua_http_intercept_ctx   *ictx;

    ictx = ts_lua_get_http_intercept_ctx(L);

    n = lua_gettop(L);

    if (n < 2) {
        return luaL_error(L, "The 2nd param of ts.fetch is not a table.\n");
    }

    url = luaL_checklstring(L, 1, &len);

    if (!lua_istable(L, 2)) {
        return luaL_error(L, "The 2nd param of ts.fetch is not a table.\n");
    }

    flags = streaming = 0;

    if (n >= 3) {
        opt = luaL_checklstring(L, 1, &opt_len);

        i = 0;

        while (i < opt_len) {
            c = *(opt + i);

            switch (c) {
                case 's':
                    streaming = 1;
                    break;

                case 'd':
                    flags |= TS_FLAG_FETCH_FORCE_DECHUNK;
                    break;

                default:
                    break;
            }
        }
    }

    contp = TSContCreate(ts_lua_fetch_handler, TSContMutexGet(ictx->contp));
    client_addr = *TSHttpTxnClientAddrGet(ictx->hctx->txnp);

    /* option */
    fch = ts_http_fetcher_create(contp, &client_addr, flags);

    /* method */
    lua_pushlstring(L, "method", sizeof("method")-1);
    lua_gettable(L, 2);

    if (lua_isstring(L, -1)) {
        method = lua_tolstring(L, -1, &method_len);
        ts_http_fetcher_init(fch, method, method_len, url, len);

    } else {
        ts_http_fetcher_init_common(fch, TS_FETCH_METHOD_GET, url, len);
    }

    lua_pop(L, 1);

    /* header */
    lua_pushlstring(L, "header", sizeof("header")-1);
    lua_gettable(L, 2);

    if (lua_istable(L, -1)) {
        lua_pushnil(L);

        while (lua_next(L, -2)) {
            value = luaL_checklstring (L, -1, &val_len);
            key = luaL_checklstring (L, -2, &key_len);

            ts_http_fetcher_add_header(fch, key, key_len, value, val_len);
            lua_pop(L, 1);
        }
    }

    lua_pop(L, 1);

    /* body */
    lua_pushlstring(L, "body", sizeof("body")-1);
    lua_gettable(L, 2);

    if (lua_isstring(L, -1)) {
        body = lua_tolstring(L, -1, &body_len);
        ts_http_fetcher_append_data(fch, body, body_len);
    }

    lua_pop(L, 1);

    size = TS_LUA_MEM_ALIGN(sizeof(ts_lua_fetch_ctx));
    fctx = (ts_lua_fetch_ctx*)TSmalloc(size);
    for (i = 0; i < TS_LUA_ALIGN_COUNT(size); i++) {
        ((void**)fctx)[i] = 0;
    }

    fctx->buffer = TSIOBufferCreate();
    fctx->reader = TSIOBufferReaderAlloc(fctx->buffer);
    fctx->ictx = ictx;
    fctx->streaming = streaming;

    item = (ts_lua_http_intercept_item*)TSmalloc(sizeof(ts_lua_http_intercept_item));
    TS_LUA_ADD_INTERCEPT_ITEM(ictx, item, contp, ts_lua_fetch_cleanup, fch);

    ts_http_fetcher_set_ctx(fch, fctx);
    TSContDataSet(contp, item);

    ts_http_fetcher_launch(fch);

    lua_settop(L, n);

    return lua_yield(L, 0);
}

static int
ts_lua_fetch_handler(TSCont contp, TSEvent event, void *edata)
{
    int64_t             avail, all;
    int64_t             blk_len, already;
    char                *data;
    const char          *start;
    const char          *name, *value;
    int                 name_len, value_len;
    lua_State           *L;
    ts_lua_fetch_ctx    *fctx;
    http_fetcher        *fch;
    TSIOBufferBlock     blk;
    TSMLoc              field_loc, next_field_loc;
    ts_lua_http_intercept_item  *item;
    ts_lua_http_intercept_ctx   *ictx;

    fch = (http_fetcher*)edata;
    fctx = (ts_lua_fetch_ctx*)ts_http_fetcher_get_ctx(fch);
    ictx = fctx->ictx;
    L = ictx->lua;

    switch (event) {

        case TS_EVENT_IMMEDIATE:
        case TS_EVENT_TIMEOUT:

            if (fctx->read_action) {
                fctx->read_action = NULL;
            }

            if (fctx->error) {
                lua_pushnil(L);         // body
                lua_pushnil(L);         // eos
                lua_pushnumber(L, 1);   // err

                TSContCall(ictx->contp, event, (void*)3);
                break;
            }

            avail = TSIOBufferReaderAvail(fch->body_reader);

            if (avail > 0) {
                /* body */
                data = TSmalloc(avail);

                already = 0;
                blk = TSIOBufferReaderStart(fch->body_reader);

                while (blk) {
                    start = TSIOBufferBlockReadStart(blk, fch->body_reader, &blk_len);
                    memcpy(data + already, start, blk_len);
                    already += blk_len;
                    blk = TSIOBufferBlockNext(blk);
                }

                lua_pushlstring(L, data, avail);

                if (fctx->body_complete) {
                    lua_pushnumber(L, 1);

                } else {
                    lua_pushnil(L);
                }

                lua_pushnil(L);         // eos

                TSContCall(ictx->contp, event, (void*)3);
                TSfree(data);

            } else if (fctx->body_complete) {
                lua_pushlstring(L, "", 0);  // body
                lua_pushnumber(L, 1);       // eos
                lua_pushnil(L);             // err

                TSContCall(ictx->contp, event, (void*)3);

            } else {
                fctx->suspended = 1;
            }

            break;

        case TS_EVENT_FETCH_HEADER_DONE:

            fctx->header_complete = 1;

            if (!fctx->streaming)
                break;

            lua_newtable(L);

            /* status */
            lua_pushlstring(L, "status", sizeof("status")-1);
            lua_pushnumber(L, fch->status_code);
            lua_settable(L, -3);

            /* header */
            lua_pushlstring(L, "header", sizeof("header")-1);
            lua_newtable(L);

            field_loc = TSMimeHdrFieldGet(fch->hdr_bufp, fch->hdr_loc, 0);
            while (field_loc) {
                name = TSMimeHdrFieldNameGet(fch->hdr_bufp, fch->hdr_loc, field_loc, &name_len);
                if (name) {
                    value = TSMimeHdrFieldValueStringGet(fch->hdr_bufp, fch->hdr_loc, field_loc, -1, &value_len);
                    lua_pushlstring(L, name, name_len);
                    lua_pushlstring(L, value, value_len);
                    lua_settable(L, -3);
                }

                next_field_loc = TSMimeHdrFieldNext(fch->hdr_bufp, fch->hdr_loc, field_loc);
                TSHandleMLocRelease(fch->hdr_bufp, fch->hdr_loc, field_loc);
                field_loc = next_field_loc;
            }

            lua_settable(L, -3);

            /* handler */
            lua_pushlstring(L, "handler", sizeof("handler")-1);
            lua_pushlightuserdata(L, fch);
            lua_settable(L, -3);

            TSContCall(ictx->contp, event, (void*)1);

            break;

        case TS_EVENT_FETCH_BODY_READY:
        case TS_EVENT_FETCH_BODY_COMPLETE:

            if (event == TS_EVENT_FETCH_BODY_COMPLETE)
                fctx->body_complete = 1;

            avail = TSIOBufferReaderAvail(fch->body_reader);

            if (fctx->streaming) {

                if (!fctx->suspended)           // wait for read
                    break;

                /* body */
                data = TSmalloc(avail);

                already = 0;
                blk = TSIOBufferReaderStart(fch->body_reader);

                while (blk) {
                    start = TSIOBufferBlockReadStart(blk, fch->body_reader, &blk_len);
                    memcpy(data + already, start, blk_len);
                    already += blk_len;
                    blk = TSIOBufferBlockNext(blk);
                }

                lua_pushlstring(L, data, avail);

                /* eos */
                if (event == TS_EVENT_FETCH_BODY_COMPLETE) {
                    lua_pushnumber(L, 1);

                } else {
                    lua_pushnil(L);
                }

                lua_pushnil(L);

                /* final */
                TSfree(data);
                ts_http_fetcher_consume_resp_body(fch, avail);

                fctx->suspended = 0;
                TSContCall(ictx->contp, event, (void*)3);

            } else {

                TSIOBufferCopy(fctx->buffer, fch->body_reader, avail, 0);
                ts_http_fetcher_consume_resp_body(fch, avail);

                if (event != TS_EVENT_FETCH_BODY_COMPLETE)
                    break;

                lua_newtable(L);

                /* status */
                lua_pushlstring(L, "status", sizeof("status")-1);
                lua_pushnumber(L, fch->status_code);
                lua_settable(L, -3);

                /* body */
                all = TSIOBufferReaderAvail(fctx->reader);
                data = TSmalloc(all);

                already = 0;
                blk = TSIOBufferReaderStart(fctx->reader);

                while (blk) {
                    start = TSIOBufferBlockReadStart(blk, fctx->reader, &blk_len);
                    memcpy(data + already, start, blk_len);
                    already += blk_len;
                    blk = TSIOBufferBlockNext(blk);
                }

                lua_pushlstring(L, "body", sizeof("body")-1);
                lua_pushlstring(L, data, all);
                lua_settable(L, -3);

                /* header */
                lua_pushlstring(L, "header", sizeof("header")-1);
                lua_newtable(L);

                field_loc = TSMimeHdrFieldGet(fch->hdr_bufp, fch->hdr_loc, 0);
                while (field_loc) {
                    name = TSMimeHdrFieldNameGet(fch->hdr_bufp, fch->hdr_loc, field_loc, &name_len);
                    if (name) {
                        value = TSMimeHdrFieldValueStringGet(fch->hdr_bufp, fch->hdr_loc, field_loc, -1, &value_len);
                        lua_pushlstring(L, name, name_len);
                        lua_pushlstring(L, value, value_len);
                        lua_settable(L, -3);
                    }

                    next_field_loc = TSMimeHdrFieldNext(fch->hdr_bufp, fch->hdr_loc, field_loc);
                    TSHandleMLocRelease(fch->hdr_bufp, fch->hdr_loc, field_loc);
                    field_loc = next_field_loc;
                }

                lua_settable(L, -3);

                /* final */
                TSfree(data);

                item = (ts_lua_http_intercept_item*)TSContDataGet(contp);
                ts_lua_fetch_cleanup(item);
                TSContCall(ictx->contp, event, (void*)1);
            }

            break;

        case TS_EVENT_FETCH_ERROR:
        default:

            fctx->error = 1;

            if (fctx->streaming) {

                if (fctx->header_complete) {     // after ts.fetch(...)

                    if (fctx->suspended) {       // block on ts.fetch_read
                        lua_pushnil(L);         // body
                        lua_pushnil(L);         // eos
                        lua_pushnumber(L, 1);   // err

                        fctx->suspended = 0;
                        TSContCall(ictx->contp, event, (void*)3);
                    }

                } else {                // block on ts.fetch(...)
                    lua_newtable(L);

                    /* err */
                    lua_pushlstring(L, "err", sizeof("err")-1);
                    lua_pushnumber(L, 1);
                    lua_settable(L, -3);

                    TSContCall(ictx->contp, event, (void*)1);
                }

            } else {

                lua_newtable(L);

                /* err */
                lua_pushlstring(L, "err", sizeof("err")-1);
                lua_pushnumber(L, 1);
                lua_settable(L, -3);

                item = (ts_lua_http_intercept_item*)TSContDataGet(contp);
                ts_lua_fetch_cleanup(item);
                TSContCall(ictx->contp, event, (void*)1);
            }

            return 0;
    }

    return 0;
}

static int
ts_lua_fetch_cleanup(struct ict_item *item)
{
    http_fetcher        *fch;
    ts_lua_fetch_ctx    *fctx;

    fch = (http_fetcher*)item->data;
    fctx = (ts_lua_fetch_ctx*)ts_http_fetcher_get_ctx(fch);

    if (fctx->read_action) {
        TSActionCancel(fctx->read_action);
        fctx->read_action = NULL;
    }

    TSIOBufferReaderFree(fctx->reader);
    TSIOBufferDestroy(fctx->buffer);

    TSfree(fctx);

    ts_http_fetcher_destroy(fch);
    TSContDestroy(item->contp);

    item->deleted = 1;
    return 0;
}

static int
ts_lua_fetch_read(lua_State *L)
{
    int                         n;
    http_fetcher                *fch;
    ts_lua_fetch_ctx            *fctx;

    n = lua_gettop(L);
    if (n < 1) {
        return luaL_error(L, "ts.fetch_read less parameter.\n");
    }

    if (!lua_istable(L, 1)) {
        return luaL_error(L, "The 1st param of ts.fetch_read is not a table.\n");
    }

    /* handler */
    lua_pushlstring(L, "handler", sizeof("handler")-1);
    lua_gettable(L, 2);

    if (lua_isuserdata(L, -1)) {
        fch = lua_touserdata(L, -1);

    } else {
        return luaL_error(L, "fetch handler not found from param of ts.fetch_read\n");
    }

    lua_pop(L, 1);

    if (!fch) {
        return luaL_error(L, "fetch handler invalid.\n");
    }

    fctx = (ts_lua_fetch_ctx*)ts_http_fetcher_get_ctx(fch);
    if (!fctx->streaming)
        return 0;

    fctx->read_action = TSContSchedule(fch->contp, 0, TS_THREAD_POOL_DEFAULT);

    return lua_yield(L, 0);
}

