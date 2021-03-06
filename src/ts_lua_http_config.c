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


#include "ts_lua_util.h"

typedef enum {
    TS_LUA_CONFIG_NULL = -1,
    TS_LUA_CONFIG_URL_REMAP_PRISTINE_HOST_HDR,
    TS_LUA_CONFIG_HTTP_CHUNKING_ENABLED,
    TS_LUA_CONFIG_HTTP_NEGATIVE_CACHING_ENABLED,
    TS_LUA_CONFIG_HTTP_NEGATIVE_CACHING_LIFETIME,
    TS_LUA_CONFIG_HTTP_CACHE_WHEN_TO_REVALIDATE,
    TS_LUA_CONFIG_HTTP_KEEP_ALIVE_ENABLED_IN,
    TS_LUA_CONFIG_HTTP_KEEP_ALIVE_ENABLED_OUT,
    TS_LUA_CONFIG_HTTP_KEEP_ALIVE_POST_OUT,
    TS_LUA_CONFIG_HTTP_SHARE_SERVER_SESSIONS,
    TS_LUA_CONFIG_NET_SOCK_RECV_BUFFER_SIZE_OUT,
    TS_LUA_CONFIG_NET_SOCK_SEND_BUFFER_SIZE_OUT,
    TS_LUA_CONFIG_NET_SOCK_OPTION_FLAG_OUT,
    TS_LUA_CONFIG_HTTP_FORWARD_PROXY_AUTH_TO_PARENT,
    TS_LUA_CONFIG_HTTP_ANONYMIZE_REMOVE_FROM,
    TS_LUA_CONFIG_HTTP_ANONYMIZE_REMOVE_REFERER,
    TS_LUA_CONFIG_HTTP_ANONYMIZE_REMOVE_USER_AGENT,
    TS_LUA_CONFIG_HTTP_ANONYMIZE_REMOVE_COOKIE,
    TS_LUA_CONFIG_HTTP_ANONYMIZE_REMOVE_CLIENT_IP,
    TS_LUA_CONFIG_HTTP_ANONYMIZE_INSERT_CLIENT_IP,
    TS_LUA_CONFIG_HTTP_RESPONSE_SERVER_ENABLED,
    TS_LUA_CONFIG_HTTP_INSERT_SQUID_X_FORWARDED_FOR,
    TS_LUA_CONFIG_HTTP_SERVER_TCP_INIT_CWND,
    TS_LUA_CONFIG_HTTP_SEND_HTTP11_REQUESTS,
    TS_LUA_CONFIG_HTTP_CACHE_HTTP,
    TS_LUA_CONFIG_HTTP_CACHE_CLUSTER_CACHE_LOCAL,
    TS_LUA_CONFIG_HTTP_CACHE_IGNORE_CLIENT_NO_CACHE,
    TS_LUA_CONFIG_HTTP_CACHE_IGNORE_CLIENT_CC_MAX_AGE,
    TS_LUA_CONFIG_HTTP_CACHE_IMS_ON_CLIENT_NO_CACHE,
    TS_LUA_CONFIG_HTTP_CACHE_IGNORE_SERVER_NO_CACHE,
    TS_LUA_CONFIG_HTTP_CACHE_CACHE_RESPONSES_TO_COOKIES,
    TS_LUA_CONFIG_HTTP_CACHE_IGNORE_AUTHENTICATION,
    TS_LUA_CONFIG_HTTP_CACHE_CACHE_URLS_THAT_LOOK_DYNAMIC,
    TS_LUA_CONFIG_HTTP_CACHE_REQUIRED_HEADERS,
    TS_LUA_CONFIG_HTTP_INSERT_REQUEST_VIA_STR,
    TS_LUA_CONFIG_HTTP_INSERT_RESPONSE_VIA_STR,
    TS_LUA_CONFIG_HTTP_CACHE_HEURISTIC_MIN_LIFETIME,
    TS_LUA_CONFIG_HTTP_CACHE_HEURISTIC_MAX_LIFETIME,
    TS_LUA_CONFIG_HTTP_CACHE_GUARANTEED_MIN_LIFETIME,
    TS_LUA_CONFIG_HTTP_CACHE_GUARANTEED_MAX_LIFETIME,
    TS_LUA_CONFIG_HTTP_CACHE_MAX_STALE_AGE,
    TS_LUA_CONFIG_HTTP_KEEP_ALIVE_NO_ACTIVITY_TIMEOUT_IN,
    TS_LUA_CONFIG_HTTP_KEEP_ALIVE_NO_ACTIVITY_TIMEOUT_OUT,
    TS_LUA_CONFIG_HTTP_TRANSACTION_NO_ACTIVITY_TIMEOUT_IN,
    TS_LUA_CONFIG_HTTP_TRANSACTION_NO_ACTIVITY_TIMEOUT_OUT,
    TS_LUA_CONFIG_HTTP_TRANSACTION_ACTIVE_TIMEOUT_OUT,
    TS_LUA_CONFIG_HTTP_ORIGIN_MAX_CONNECTIONS,
    TS_LUA_CONFIG_HTTP_CONNECT_ATTEMPTS_MAX_RETRIES,
    TS_LUA_CONFIG_HTTP_CONNECT_ATTEMPTS_MAX_RETRIES_DEAD_SERVER,
    TS_LUA_CONFIG_HTTP_CONNECT_ATTEMPTS_RR_RETRIES,
    TS_LUA_CONFIG_HTTP_CONNECT_ATTEMPTS_TIMEOUT,
    TS_LUA_CONFIG_HTTP_POST_CONNECT_ATTEMPTS_TIMEOUT,
    TS_LUA_CONFIG_HTTP_DOWN_SERVER_CACHE_TIME,
    TS_LUA_CONFIG_HTTP_DOWN_SERVER_ABORT_THRESHOLD,
    TS_LUA_CONFIG_HTTP_CACHE_FUZZ_TIME,
    TS_LUA_CONFIG_HTTP_CACHE_FUZZ_MIN_TIME,
    TS_LUA_CONFIG_HTTP_DOC_IN_CACHE_SKIP_DNS,
    TS_LUA_CONFIG_HTTP_RESPONSE_SERVER_STR,
    TS_LUA_CONFIG_HTTP_CACHE_HEURISTIC_LM_FACTOR,
    TS_LUA_CONFIG_HTTP_CACHE_FUZZ_PROBABILITY,
    TS_LUA_CONFIG_NET_SOCK_PACKET_MARK_OUT,
    TS_LUA_CONFIG_NET_SOCK_PACKET_TOS_OUT,
    TS_LUA_CONFIG_LAST_ENTRY
} TSLuaOverridableConfigKey;


char * ts_lua_http_config_string[]  = {
    "TS_LUA_CONFIG_URL_REMAP_PRISTINE_HOST_HDR",
    "TS_LUA_CONFIG_HTTP_CHUNKING_ENABLED",
    "TS_LUA_CONFIG_HTTP_NEGATIVE_CACHING_ENABLED",
    "TS_LUA_CONFIG_HTTP_NEGATIVE_CACHING_LIFETIME",
    "TS_LUA_CONFIG_HTTP_CACHE_WHEN_TO_REVALIDATE",
    "TS_LUA_CONFIG_HTTP_KEEP_ALIVE_ENABLED_IN",
    "TS_LUA_CONFIG_HTTP_KEEP_ALIVE_ENABLED_OUT",
    "TS_LUA_CONFIG_HTTP_KEEP_ALIVE_POST_OUT",
    "TS_LUA_CONFIG_HTTP_SHARE_SERVER_SESSIONS",
    "TS_LUA_CONFIG_NET_SOCK_RECV_BUFFER_SIZE_OUT",
    "TS_LUA_CONFIG_NET_SOCK_SEND_BUFFER_SIZE_OUT",
    "TS_LUA_CONFIG_NET_SOCK_OPTION_FLAG_OUT",
    "TS_LUA_CONFIG_HTTP_FORWARD_PROXY_AUTH_TO_PARENT",
    "TS_LUA_CONFIG_HTTP_ANONYMIZE_REMOVE_FROM",
    "TS_LUA_CONFIG_HTTP_ANONYMIZE_REMOVE_REFERER",
    "TS_LUA_CONFIG_HTTP_ANONYMIZE_REMOVE_USER_AGENT",
    "TS_LUA_CONFIG_HTTP_ANONYMIZE_REMOVE_COOKIE",
    "TS_LUA_CONFIG_HTTP_ANONYMIZE_REMOVE_CLIENT_IP",
    "TS_LUA_CONFIG_HTTP_ANONYMIZE_INSERT_CLIENT_IP",
    "TS_LUA_CONFIG_HTTP_RESPONSE_SERVER_ENABLED",
    "TS_LUA_CONFIG_HTTP_INSERT_SQUID_X_FORWARDED_FOR",
    "TS_LUA_CONFIG_HTTP_SERVER_TCP_INIT_CWND",
    "TS_LUA_CONFIG_HTTP_SEND_HTTP11_REQUESTS",
    "TS_LUA_CONFIG_HTTP_CACHE_HTTP",
    "TS_LUA_CONFIG_HTTP_CACHE_CLUSTER_CACHE_LOCAL",
    "TS_LUA_CONFIG_HTTP_CACHE_IGNORE_CLIENT_NO_CACHE",
    "TS_LUA_CONFIG_HTTP_CACHE_IGNORE_CLIENT_CC_MAX_AGE",
    "TS_LUA_CONFIG_HTTP_CACHE_IMS_ON_CLIENT_NO_CACHE",
    "TS_LUA_CONFIG_HTTP_CACHE_IGNORE_SERVER_NO_CACHE",
    "TS_LUA_CONFIG_HTTP_CACHE_CACHE_RESPONSES_TO_COOKIES",
    "TS_LUA_CONFIG_HTTP_CACHE_IGNORE_AUTHENTICATION",
    "TS_LUA_CONFIG_HTTP_CACHE_CACHE_URLS_THAT_LOOK_DYNAMIC",
    "TS_LUA_CONFIG_HTTP_CACHE_REQUIRED_HEADERS",
    "TS_LUA_CONFIG_HTTP_INSERT_REQUEST_VIA_STR",
    "TS_LUA_CONFIG_HTTP_INSERT_RESPONSE_VIA_STR",
    "TS_LUA_CONFIG_HTTP_CACHE_HEURISTIC_MIN_LIFETIME",
    "TS_LUA_CONFIG_HTTP_CACHE_HEURISTIC_MAX_LIFETIME",
    "TS_LUA_CONFIG_HTTP_CACHE_GUARANTEED_MIN_LIFETIME",
    "TS_LUA_CONFIG_HTTP_CACHE_GUARANTEED_MAX_LIFETIME",
    "TS_LUA_CONFIG_HTTP_CACHE_MAX_STALE_AGE",
    "TS_LUA_CONFIG_HTTP_KEEP_ALIVE_NO_ACTIVITY_TIMEOUT_IN",
    "TS_LUA_CONFIG_HTTP_KEEP_ALIVE_NO_ACTIVITY_TIMEOUT_OUT",
    "TS_LUA_CONFIG_HTTP_TRANSACTION_NO_ACTIVITY_TIMEOUT_IN",
    "TS_LUA_CONFIG_HTTP_TRANSACTION_NO_ACTIVITY_TIMEOUT_OUT",
    "TS_LUA_CONFIG_HTTP_TRANSACTION_ACTIVE_TIMEOUT_OUT",
    "TS_LUA_CONFIG_HTTP_ORIGIN_MAX_CONNECTIONS",
    "TS_LUA_CONFIG_HTTP_CONNECT_ATTEMPTS_MAX_RETRIES",
    "TS_LUA_CONFIG_HTTP_CONNECT_ATTEMPTS_MAX_RETRIES_DEAD_SERVER",
    "TS_LUA_CONFIG_HTTP_CONNECT_ATTEMPTS_RR_RETRIES",
    "TS_LUA_CONFIG_HTTP_CONNECT_ATTEMPTS_TIMEOUT",
    "TS_LUA_CONFIG_HTTP_POST_CONNECT_ATTEMPTS_TIMEOUT",
    "TS_LUA_CONFIG_HTTP_DOWN_SERVER_CACHE_TIME",
    "TS_LUA_CONFIG_HTTP_DOWN_SERVER_ABORT_THRESHOLD",
    "TS_LUA_CONFIG_HTTP_CACHE_FUZZ_TIME",
    "TS_LUA_CONFIG_HTTP_CACHE_FUZZ_MIN_TIME",
    "TS_LUA_CONFIG_HTTP_DOC_IN_CACHE_SKIP_DNS",
    "TS_LUA_CONFIG_HTTP_RESPONSE_SERVER_STR",
    "TS_LUA_CONFIG_HTTP_CACHE_HEURISTIC_LM_FACTOR",
    "TS_LUA_CONFIG_HTTP_CACHE_FUZZ_PROBABILITY",
    "TS_LUA_CONFIG_NET_SOCK_PACKET_MARK_OUT",
    "TS_LUA_CONFIG_NET_SOCK_PACKET_TOS_OUT",
    "TS_LUA_CONFIG_LAST_ENTRY",
};

static void ts_lua_inject_http_config_variables(lua_State *L);

static int ts_lua_http_config_int_set(lua_State *L);
static int ts_lua_http_config_int_get(lua_State *L);
static int ts_lua_http_config_float_set(lua_State *L);
static int ts_lua_http_config_float_get(lua_State *L);
static int ts_lua_http_config_string_set(lua_State *L);
static int ts_lua_http_config_string_get(lua_State *L);


void
ts_lua_inject_http_config_api(lua_State *L)
{
    ts_lua_inject_http_config_variables(L);

    lua_pushcfunction(L, ts_lua_http_config_int_set);
    lua_setfield(L, -2, "config_int_set");

    lua_pushcfunction(L, ts_lua_http_config_int_get);
    lua_setfield(L, -2, "config_int_get");

    lua_pushcfunction(L, ts_lua_http_config_float_set);
    lua_setfield(L, -2, "config_float_set");

    lua_pushcfunction(L, ts_lua_http_config_float_get);
    lua_setfield(L, -2, "config_float_get");

    lua_pushcfunction(L, ts_lua_http_config_string_set);
    lua_setfield(L, -2, "config_string_set");

    lua_pushcfunction(L, ts_lua_http_config_string_get);
    lua_setfield(L, -2, "config_string_get");
}

static void
ts_lua_inject_http_config_variables(lua_State *L)
{
    int     i;

    for (i = 0; i <= TS_LUA_CONFIG_LAST_ENTRY; i++) {
        lua_pushinteger(L, i);
        lua_setglobal(L, ts_lua_http_config_string[i]);
    }

}

static int
ts_lua_http_config_int_set(lua_State *L)
{
    int                 conf;
    int                 value;
    ts_lua_http_ctx     *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    conf = luaL_checkinteger(L, 1);
    value = luaL_checkinteger(L, 2);

    TSHttpTxnConfigIntSet(http_ctx->txnp, conf, value);

    return 0;
}

static int
ts_lua_http_config_int_get(lua_State *L)
{
    int                 conf;
    int64_t             value;
    ts_lua_http_ctx     *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    conf = luaL_checkinteger(L, 1);

    TSHttpTxnConfigIntGet(http_ctx->txnp, conf, &value);

    lua_pushnumber(L, value);

    return 1;
}

static int
ts_lua_http_config_float_set(lua_State *L)
{
    int                 conf;
    float               value;
    ts_lua_http_ctx     *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    conf = luaL_checkinteger(L, 1);
    value = luaL_checknumber(L, 2);

    TSHttpTxnConfigFloatSet(http_ctx->txnp, conf, value);

    return 0;
}

static int
ts_lua_http_config_float_get(lua_State *L)
{
    int                 conf;
    float               value;
    ts_lua_http_ctx     *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    conf = luaL_checkinteger(L, 1);

    TSHttpTxnConfigFloatGet(http_ctx->txnp, conf, &value);

    lua_pushnumber(L, value);

    return 1;
}

static int
ts_lua_http_config_string_set(lua_State *L)
{
    int                 conf;
    const char          *value;
    size_t              value_len;
    ts_lua_http_ctx     *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    conf = luaL_checkinteger(L, 1);
    value = luaL_checklstring(L, 2, &value_len);

    TSHttpTxnConfigStringSet(http_ctx->txnp, conf, value, value_len);

    return 0;
}

static int
ts_lua_http_config_string_get(lua_State *L)
{
    int                 conf;
    const char          *value;
    int                 value_len;
    ts_lua_http_ctx     *http_ctx;

    http_ctx = ts_lua_get_http_ctx(L);

    conf = luaL_checkinteger(L, 1);

    TSHttpTxnConfigStringGet(http_ctx->txnp, conf, &value, &value_len);

    lua_pushlstring(L, value, value_len);

    return 1;
}

