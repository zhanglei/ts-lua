
AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = tslua.so

tslua_so_SOURCES = ts_lua.c ts_lua_string.c ts_lua_client_request.c  ts_lua_client_response.c ts_lua_server_request.c \
                           ts_lua_server_response.c  ts_lua_cached_response.c ts_lua_util.c ts_lua_remap.c \
                           ts_lua_hook.c ts_lua_context.c ts_lua_misc.c ts_lua_http.c ts_lua_transform.c ts_lua_log.c \
                           ts_lua_http_intercept.c ts_lua_regex.c ts_lua_http_config.c ts_lua_crypto.c ts_lua_hash_table.c
 
tslua_so_CPPFLAGS = -g -fPIC

tslua_so_LDFLAGS = -shared

tslua_so_LDADD = $(LUA_LIBS)

