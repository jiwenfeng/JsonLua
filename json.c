#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <ctype.h>
#include <lualib.h>

static const char *parse_value(lua_State *L, const char *str);
static const char *parse_string(lua_State *L, const char *str);
static const char *parse_object(lua_State *L, const char *str);
static const char *parse_array(lua_State *L, const char *str);
static const char *parse_number(lua_State *L, const char *str);
static const char *skip_space(const char *str);
static int is_array(lua_State *L, int idx);
static int ldecode(lua_State *L);
static int lencode(lua_State *L);
static int encode(lua_State *L, luaL_Buffer *buffer);
static int encode_object(lua_State *L, luaL_Buffer *buffer);
static int encode_array(lua_State *L, luaL_Buffer *buffer);

static const char *
skip_space(const char *str)
{
	while(str && isspace(*str))
	{
		++str;
	}
	return str;
}

static const char *
parse_number(lua_State *L, const char *str)
{
	double n = 0, sign = 1, scale = 0;
	int subscale = 0, signsubscale = 1;
	if(*str == '-')
	{
		sign = -1;
		str++;
	}
	if(*str == '0')
	{
		str++;
	}
	if(*str >= '1' && *str <= '9')
	{
		do
		{
			n = (n * 10.0) + (*str++ - '0');
		} while(*str >= '0' && *str <= '9');
	}
	if(*str == '.')
	{
		str++;
		do
		{
			n = (n * 10.0) + (*str++ - '0');
			scale--;
		} while(*str >= '0' && *str <= '9');
	}
	if(*str == 'e' || *str == 'E')
	{
		str++;
		if(*str == '+')
		{
			str++;
		}
		else
		{
			if(*str == '-')
			{
				signsubscale = -1;
				str++;
			}
		}
		while(*str >= '0' && *str <= '9')
		{
			subscale = (subscale * 10) + (*str++ - '0');
		}
	}
	n = sign * n * pow(10.0, (scale + subscale * signsubscale));
	lua_pushnumber(L, n);
	return str;
}

static const char *
parse_string(lua_State *L, const char *str)
{
	if(*str != '"')
	{
		lua_pushstring(L, "Parse String Error:Expecting '\"'");
		return NULL;
	}
	int i = 1;
	while(*(str + i) != '"')
	{
		i++;
	}
	lua_pushlstring(L, str + 1, i - 1);
	i++;
	return str + i;
}

static const char *
parse_array(lua_State *L, const char *str)
{
	if(*str != '[')
	{
		lua_pushstring(L, "Parse String Error:Expecting '['");
		return NULL;
	}
	str = skip_space(str + 1);
	if(NULL == str)
	{
		lua_pushstring(L, "Parse String Error:Expecting ']'");
		return NULL;
	}
	lua_newtable(L);
	str = skip_space(parse_value(L, str));
	int index = 1;
	while(*str == ',')
	{
		lua_rawseti(L, -2, index);
		++index;
		str = skip_space(parse_string(L, skip_space(str + 1)));
		if(str == NULL)
		{
			lua_pushstring(L, "Parse String Error:Expecting 'STRING'");
			return NULL;
		}
	}	
	if(*str != ']')
	{
		lua_pushstring(L, "Parse String Error:Expecting ']'");
		return NULL;
	}
	lua_rawseti(L, -2, index);
	return str;
}

static const char *
parse_object(lua_State *L, const char *str)
{
	if(*str != '{')
	{
		lua_pushstring(L, "Parse String Error:Expecting '{'");
		return NULL;
	}
	str = skip_space(str + 1);
	if(NULL == str)
	{
		lua_pushstring(L, "Parse String Error:Expecting 'STRING'");
		return NULL;
	}
	if(*str == '}')
	{
		lua_newtable(L);
		return str++;
	}
	if(*str != '"')
	{
		lua_pushstring(L, "Parse String Error:Expecting '\"'");
		return NULL;
	}
	lua_newtable(L);
	str = skip_space(parse_string(L, str));
	if(NULL == str || *str != ':')
	{
		lua_pushstring(L, "Parse String Error:Expecting ':'");
		return NULL;
	}
	str = skip_space(parse_value(L, str + 1));
	if(NULL == str)
	{
		lua_pushstring(L, "Parse String Error:Expecting 'STRING'");
		return NULL;
	}
	while(*str == ',')
	{
		lua_settable(L, -3);
		str = skip_space(parse_string(L, skip_space(str + 1)));
		if(str == NULL || *str != ':')
		{
			lua_pushstring(L, "Parse String Error:Expecting ':'");
			return NULL;
		}
		str = skip_space(parse_value(L, str + 1));
		if(str == NULL)
		{
			lua_pushstring(L, "Parse String Error:Expecting 'STRING'");
			return NULL;
		}
	}
	if(*str == '}')
	{
		lua_settable(L, -3);
		return str + 1;
	}
	return str;
}

static const char *
parse_value(lua_State *L, const char *str)
{
	str = skip_space(str);
	if(*str == '{')
	{
		return parse_object(L, str);
	}
	if(*str == '[')
	{
		return parse_array(L, str);
	}
	if(*str == '"')
	{
		return parse_string(L, str);
	}
	if(*str == '-' || (*str >= '0' &&  *str <= '9'))
	{
		return parse_number(L, str);
	}
	lua_pushstring(L, "Parse String Error:Expecting '{', '['");
	return NULL;
}

static int
ldecode(lua_State *L)
{
	luaL_argcheck(L, lua_type(L, -1) == LUA_TSTRING, 1, "String Expected");
	int top = lua_gettop(L);
	const char *str = lua_tostring(L, -1);
	str = parse_value(L, str);
	if(NULL == str)
	{
		printf("%s\n", lua_tostring(L, -1));
		lua_settop(L, top);
		return 0;
	}
	return 1;
}

static int
is_array(lua_State *L, int idx)
{
	lua_pushvalue(L, idx);
	if(!lua_istable(L, -1))
	{
		luaL_error(L, "table expected got `%s'", lua_typename(L, lua_type(L, idx)));
	}
	lua_len(L, -1);
	int sz = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pushnil(L);
	int n = 0;
	while(lua_next(L, -2))
	{
		n++;
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return n == sz;
}

static int
encode_object(lua_State *L, luaL_Buffer *buffer)
{
	lua_pushstring(L, "{");
	luaL_addvalue(buffer);
	int flag = 0;
	lua_pushnil(L);
	while(lua_next(L, -2))
	{
		if(flag)
		{
			lua_pushstring(L, ",");
			luaL_addvalue(buffer);
		}
		flag = 1;
		switch(lua_type(L, -2))
		{
			case LUA_TSTRING:
				lua_pushfstring(L, "\"%s\":", lua_tostring(L, -2));
				luaL_addvalue(buffer);
				break;
			case LUA_TNUMBER:
			{
				char buf[1024] = {0};
				snprintf(buf, 1024, "\"%g\":", lua_tonumber(L, -2));
				luaL_addstring(buffer, buf);
				break;
			}
			default:
				luaL_error(L, "Unsupport key type:%s", lua_typename(L, lua_type(L, -2)));
				return -1;
		}
		switch(lua_type(L, -1))
		{
			case LUA_TSTRING:
				lua_pushfstring(L, "\"%s\"", lua_tostring(L, -1));
				luaL_addvalue(buffer);
				break;
			case LUA_TBOOLEAN:
			{
				lua_pushfstring(L, "%d", lua_toboolean(L, -1));
				luaL_addvalue(buffer);
				break;
			}
			case LUA_TNUMBER:
			{
				char buf[1024] = {0};
				snprintf(buf, 1024, "%g", lua_tonumber(L, -1));
				luaL_addstring(buffer, buf);
				break;
			}
			case LUA_TTABLE:
				encode(L, buffer);
				break;
			default:
				luaL_error(L, "Unsupport value type:%s", lua_typename(L, lua_type(L, -1)));
				return -1;
		}
		lua_pop(L, 1);
	}
	lua_pushstring(L, "}");
	luaL_addvalue(buffer);
	return 0;
}

static int
encode_array(lua_State *L, luaL_Buffer *buffer)
{
	lua_pushstring(L, "[");
	luaL_addvalue(buffer);
	int flag = 0;
	lua_pushnil(L);
	while(lua_next(L, -2))
	{
		if(flag)
		{
			lua_pushstring(L, ",");
			luaL_addvalue(buffer);
		}
		flag = 1;
		char buf[1024] = {0};
		switch(lua_type(L, -1))
		{
			case LUA_TSTRING:
				lua_pushfstring(L, "\"%s\"", lua_tostring(L, -1));
				luaL_addvalue(buffer);
				break;
			case LUA_TBOOLEAN:
				lua_pushfstring(L, "%d", lua_toboolean(L, -1));
				luaL_addvalue(buffer);
				break;
			case LUA_TNUMBER:
				snprintf(buf, 1024, "%g", lua_tonumber(L, -1));
				luaL_addstring(buffer, buf);
				break;
			case LUA_TTABLE:
				encode(L, buffer);
				break;
			default:
				luaL_error(L, "Unsupport type:%s", lua_typename(L, lua_type(L, -1)));
				return -1;
		}
		lua_pop(L, 1);
	}
	lua_pushstring(L, "]");
	luaL_addvalue(buffer);
	return 0;
}

static int
encode(lua_State *L, luaL_Buffer *buffer)
{
	int type = lua_type(L, -1);
	char buf[1024] = {0};
	switch(type)
	{
		case LUA_TBOOLEAN:
			lua_pushfstring(L, "\"%d\"", lua_toboolean(L, -1));
			luaL_addvalue(buffer);
			break;
		case LUA_TNUMBER:
			snprintf(buf, 1024, "\"%g\"", lua_tonumber(L, -1));
			luaL_addstring(buffer, buf);
			break;
		case LUA_TSTRING:
			lua_pushfstring(L, "\"%s\"", lua_tostring(L, -1));
			luaL_addvalue(buffer);
			break;
		case LUA_TTABLE:
		{
			int ret = is_array(L, -1) ? encode_array(L, buffer) : encode_object(L, buffer);
			(void)ret; // avoid warning
			break;
		}
		default:
			luaL_error(L, "Not support field type:`%s'", lua_typename(L, type));
			return -1;
	}
	return 0;
}

static int
lencode(lua_State *L)
{
	luaL_Buffer buffer;
	luaL_buffinit(L, &buffer);
	encode(L, &buffer);
	luaL_pushresult(&buffer);
	return 1;
}

static const luaL_Reg json[] = 
{
	{"Encode", lencode},
	{"Decode", ldecode},
	{NULL, NULL}
};

extern int
luaopen_json(lua_State *L)
{
	luaL_newlib(L, json);
	return 1;
}

int 
main()
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	luaL_requiref(L, "json", luaopen_json, 0);
	if(LUA_OK != luaL_dofile(L, "main.lua"))
	{
		printf("%s\n", lua_tostring(L, -1));
		return 0;
	}
	lua_close(L);
	return 0;
}
