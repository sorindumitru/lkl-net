%{
#include "../include/tokens.h"
%}
%option nounput
%option noinput
DIGIT	[0-9]
LETTER	[a-zA-Z]
HEX	[0-9a-fA-F]
INTERFACE	"interface"
TOPOLOGY		"topology"
BRIDGE		"bridge"
START		"{"
END		"}"
T_IPADDRESS	"ipaddress"
T_GATEWAY	"gateway"
T_MAC		"mac"
T_DEV		"dev"
T_PORT		"port"
IPADDRESS	(([0-9]){1,3}"."){3}([0-9]){1,3}
NETMASK	"/"(3[0-2]|[0-2]?[0-9])
MAC	({HEX}{HEX}:){5}{HEX}{HEX}
DEV	{LETTER}{3}{DIGIT}+
PORT	[1-9]{DIGIT}{0,5}
%%
{INTERFACE}	return TOK_INTERFACE;
{TOPOLOGY}	return TOK_TOPOLOGY;
{BRIDGE}	return TOK_BRIDGE;
{START}		return TOK_START;
{END}		return TOK_END;
{T_IPADDRESS}	//ipaddress
{T_GATEWAY}	//gateway
{T_MAC}		//mac
{T_DEV}		//dev
{T_PORT}	//port
{IPADDRESS}	return TOK_IPADDRESS;
{NETMASK}	return TOK_NETMASK;
{MAC}		return TOK_MAC;
{DEV}		return TOK_DEV;
{PORT}		return TOK_PORT;
";"	//
\n	//
\t	//
.	//
%%
int yywrap(){
	return 1;
}
