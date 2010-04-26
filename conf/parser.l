	int num_lines = 0;
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
HUB		"hub"
ROUTER		"router"
T_DEVICE	"device"
T_IPADDRESS	"ipaddress"
T_GATEWAY	"gateway"
T_MAC		"mac"
T_DEV		"dev"
T_PORT		"port"
T_SWITCH	"switch"
T_DEFAULT	"default"
T_HOSTNAME	"hostname"
T_HYPERVISOR	"hypervisor"
T_CONFIG	"config"
T_TYPE		"type"
IPADDRESS	(([0-9]){1,3}"."){3}([0-9]){1,3}
NETMASK	"/"(3[0-2]|[0-2]?[0-9])
MAC	({HEX}{HEX}:){5}{HEX}{HEX}
DEV	{LETTER}{3}{DIGIT}+
PORT	[1-9]{DIGIT}{0,5}
HOSTNAME ({LETTER}|{DIGIT}|_)+
CONFIG_FILE ({LETTER}|{DIGIT}|"/"|"_"|".")+
%%
{ROUTER}	return TOK_ROUTER;
{HUB}		return TOK_HUB;
{INTERFACE}	return TOK_INTERFACE;
{TOPOLOGY}	return TOK_TOPOLOGY;
{BRIDGE}	return TOK_BRIDGE;
{START}		return TOK_START;
{END}		return TOK_END;
{T_IPADDRESS}	return TOK_T_IPADDRESS;
{T_GATEWAY}	return TOK_T_GATEWAY;
{T_MAC}		return TOK_T_MAC;
{T_DEV}		return TOK_T_DEV;
{T_PORT}	return TOK_T_PORT;
{T_SWITCH}	return TOK_SWITCH;
{T_DEFAULT}	return TOK_T_DEFAULT;
{T_HOSTNAME}	return TOK_T_HOSTNAME;
{T_HYPERVISOR}	return TOK_T_HYPERVISOR;
{T_TYPE}	return TOK_T_TYPE;
{T_DEVICE}	return TOK_T_DEVICE;
{T_CONFIG}	return TOK_T_CONFIG;
{IPADDRESS}	return TOK_IPADDRESS;
{NETMASK}	return TOK_NETMASK;
{MAC}		return TOK_MAC;
{DEV}		return TOK_DEV;
{PORT}		return TOK_PORT;
{HOSTNAME}	return TOK_HOSTNAME;
{CONFIG_FILE}	return TOK_CONFIG_FILE;
";"	//
\n	num_lines++;
\t	//
.	//
%%
int yywrap(){
	return 1;
}
