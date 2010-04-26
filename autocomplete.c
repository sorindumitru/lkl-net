#include <string.h>
#include <stdlib.h>
#include <console.h>

char cc[100];
extern command *com;
extern char *prompt;
void add_newline(char **matches, int num_matches, int max_length){
	int i;
	for(i=1;i<=num_matches;i++){
		printf("\n\t\t%s",matches[i]);
	}
	printf("\n%s%s",prompt,rl_line_buffer);
}

char *complete_other_words(const char *text,int state)
{
	static int list_index, len,rest,rest_of_cmd_size;
	char *name,*rest_of_cmd;
	if (!state) {
		list_index = 0;
		len = strlen (text);
	}
	while (list_index < cmd->cmds_no) {
		name = cmd->cmds[list_index++];
		if (strncmp (name, rl_line_buffer, strlen(rl_line_buffer)) == 0){
			rest=strlen(name)-strlen(rl_line_buffer);
			rest_of_cmd_size = rest + strlen(text);
			if (strcmp(rl_line_buffer,"")!=0){
				rest_of_cmd=(char*)malloc((1+rest_of_cmd_size)*sizeof(char));
				memset(rest_of_cmd,0,(1+rest_of_cmd_size)*sizeof(char));
			}else{
				rest_of_cmd_size +=strlen(cmd->doc[list_index-1]);
				rest_of_cmd=(char*)malloc((4+rest_of_cmd_size)*sizeof(char));
				memset(rest_of_cmd,0,4+rest_of_cmd_size*sizeof(char));
			}
			memcpy(rest_of_cmd,text,strlen(text)*sizeof(char));
			memcpy(rest_of_cmd+strlen(text),name+strlen(rl_line_buffer),rest*sizeof(char));
			if (strcmp(rl_line_buffer,"")==0){
				rest_of_cmd[strlen(rest_of_cmd)]=' ';
				rest_of_cmd[strlen(rest_of_cmd)]=':';
				rest_of_cmd[strlen(rest_of_cmd)]=' ';
				memcpy(rest_of_cmd+strlen(rest_of_cmd),cmd->doc[list_index-1],strlen(cmd->doc[list_index-1])*sizeof(char));
				rest_of_cmd[strlen(rest_of_cmd)]='\0';
			}
			rest_of_cmd[strlen(rest_of_cmd)]='\0';
			return rest_of_cmd;
		}
	}

    /* If no names matched, then return NULL. */
	return ((char *)NULL);
}

void list_commands(command* c, int i)
{
	int len;
	if(c[i].name!=NULL){
		len = strlen(cc);
		memcpy(cc+len,c[i].name,strlen(c[i].name)*sizeof(char));
		cc[strlen(cc)]=' ';
		if (c[i].children==NULL){
			if (c[i].parameters!=NULL){
				memcpy(cc+strlen(cc),c[i].parameters,strlen(c[i].parameters));
			}
		cmd->cmds[cmd->cmds_no]=(char*)malloc(strlen(cc)+1);
		memset(cmd->cmds[cmd->cmds_no],0,strlen(cc)+1);
		memcpy(cmd->cmds[cmd->cmds_no],cc,strlen(cc));
		//add documentation
		cmd->doc[cmd->cmds_no]=(char*)malloc(strlen(c[i].documentation)+1);
		memset(cmd->doc[cmd->cmds_no],0,strlen(c[i].documentation)+1);
		memcpy(cmd->doc[cmd->cmds_no],c[i].documentation,strlen(c[i].documentation));
		cmd->cmds_no++;
		memset(cc+len,0,100-len);

		}else if (c[i].children!=NULL){
			list_commands(c[i].children,0);
		}

		memset(cc+len,0,100-len); 
		list_commands(c,++i);	
	}
}

void initialize_autocomplete( void )
{
	int i;

	cmd=(all_commands*)malloc(sizeof(all_commands));
	cmd->cmds_no = 0;
	cmd->cmds=(char**)malloc(MAX_COMMAND_NO*sizeof(char*));
	memset(cmd->cmds,0,MAX_COMMAND_NO);
	cmd->doc=(char**)malloc(MAX_COMMAND_NO*sizeof(char*));
	memset(cmd->doc,0,MAX_COMMAND_NO);
	list_commands(com,0);
	rl_completion_display_matches_hook = add_newline;
	rl_completion_entry_function = complete_other_words;
}
