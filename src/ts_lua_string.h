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


#ifndef _TS_LUA_STRING_H
#define _TS_LUA_STRING_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define ts_lua_base64_encoded_length(len)  (((len + 2) / 3) * 4)
#define ts_lua_base64_decoded_length(len)  (((len + 3) / 4) * 3)

u_char * ts_lua_hex_dump(u_char *dst, u_char *src, size_t len);

void ts_lua_encode_base64(u_char *dst, size_t *dst_len, u_char *src, size_t src_len);
int ts_lua_decode_base64(u_char *dst, size_t *dst_len, u_char *src, size_t src_len);

#endif

