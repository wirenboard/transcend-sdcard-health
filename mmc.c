/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 *
 * Modified to add field firmware update support,
 * those modifications are Copyright (c) 2016 SanDisk Corp.
 *
 * (This code is based on btrfs-progs/btrfs.c.)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mmc_cmds.h"

#define MMC_VERSION	"0.1"

#define BASIC_HELP 0
#define ADVANCED_HELP 1

typedef int (*CommandFunction)(int argc, char **argv);

struct Command {
	CommandFunction	func;	/* function which implements the command */
	int	nargs;		/* if == 999, any number of arguments
				   if >= 0, number of arguments,
				   if < 0, _minimum_ number of arguments */
	char	*verb;		/* verb */
	char	*help;		/* help lines; from the 2nd line onward they 
                                   are automatically indented */
        char    *adv_help;      /* advanced help message; from the 2nd line 
                                   onward they are automatically indented */

	/* the following fields are run-time filled by the program */
	char	**cmds;		/* array of subcommands */
	int	ncmds;		/* number of subcommand */
};

static struct Command commands[] = {
	/*
	 *	avoid short commands different for the case only
	 */
	{ do_SMART_buffer_dump, -1,/* Get SMART buffer */
		"smtbuffer", "<device>\n",
		NULL
	},
	{ show_SMART_info, -1,     /* Get SMART info */
		"smart", "<device>\n",
		NULL
	},
	{ show_Health_info, -1,    /* Get Health info*/
		"health", "<device>\n",
		NULL
	},
	{ 0, 0, 0, 0 }
};

static char *get_prgname(char *programname)
{
	char	*np;
	np = strrchr(programname,'/');
	if(!np)
		np = programname;
	else
		np++;

	return np;
}

static void print_help(char *programname, struct Command *cmd, int helptype)
{
	char	*pc;

	printf("\t%s %s ", programname, cmd->verb );

	if (helptype == ADVANCED_HELP && cmd->adv_help)
		for(pc = cmd->adv_help; *pc; pc++){
			putchar(*pc);
			if(*pc == '\n')
				printf("\t\t");
		}
	else
		for(pc = cmd->help; *pc; pc++){
			putchar(*pc);
			if(*pc == '\n')
				printf("\t\t");
		}

	putchar('\n');
}

static void help(char *np)
{
	struct Command *cp;

	printf("Usage:\n");
	for( cp = commands; cp->verb; cp++ )
		print_help(np, cp, BASIC_HELP);

	printf("\n\t%s help|--help|-h\n\t\tShow the help.\n",np);
	printf("\n\t%s <cmd> --help\n\t\tShow detailed help for a command or subset of commands.\n",np);
	printf("\n%s\n", MMC_VERSION);
}

static int split_command(char *cmd, char ***commands)
{
	int	c, l;
	char	*p, *s;

	for( *commands = 0, l = c = 0, p = s = cmd ; ; p++, l++ ){
		if ( *p && *p != ' ' )
			continue;

		/* c + 2 so that we have room for the null */
		(*commands) = realloc( (*commands), sizeof(char *)*(c + 2));
		(*commands)[c] = strndup(s, l);
		c++;
		l = 0;
		s = p+1;
		if( !*p ) break;
	}

	(*commands)[c] = 0;
	return c;
}

/*
	This function checks if the passed command is ambiguous
*/
static int check_ambiguity(struct Command *cmd, char **argv){
	int		i;
	struct Command	*cp;
	/* check for ambiguity */
	for( i = 0 ; i < cmd->ncmds ; i++ ){
		int match;
		for( match = 0, cp = commands; cp->verb; cp++ ){
			int	j, skip;
			char	*s1, *s2;

			if( cp->ncmds < i )
				continue;

			for( skip = 0, j = 0 ; j < i ; j++ )
				if( strcmp(cmd->cmds[j], cp->cmds[j])){
					skip=1;
					break;
				}
			if(skip)
				continue;

			if( !strcmp(cmd->cmds[i], cp->cmds[i]))
				continue;
			for(s2 = cp->cmds[i], s1 = argv[i+1];
				*s1 == *s2 && *s1; s1++, s2++ ) ;
			if( !*s1 )
				match++;
		}
		if(match){
			int j;
			fprintf(stderr, "ERROR: in command '");
			for( j = 0 ; j <= i ; j++ )
				fprintf(stderr, "%s%s",j?" ":"", argv[j+1]);
			fprintf(stderr, "', '%s' is ambiguous\n",argv[j]);
			return -2;
		}
	}
	return 0;
}

/*
 * This function, compacts the program name and the command in the first
 * element of the '*av' array
 */
static int prepare_args(int *ac, char ***av, char *prgname, struct Command *cmd ){

	char	**ret;
	int	i;
	char	*newname;

	ret = (char **)malloc(sizeof(char*)*(*ac+1));
	newname = (char*)malloc(strlen(prgname)+strlen(cmd->verb)+2);
	if( !ret || !newname ){
		free(ret);
		free(newname);
		return -1;
	}

	ret[0] = newname;
	for(i=0; i < *ac ; i++ )
		ret[i+1] = (*av)[i];

	strcpy(newname, prgname);
	strcat(newname, " ");
	strcat(newname, cmd->verb);

	(*ac)++;
	*av = ret;

	return 0;

}

/*
	This function performs the following jobs:
	- show the help if '--help' or 'help' or '-h' are passed
	- verify that a command is not ambiguous, otherwise show which
	  part of the command is ambiguous
	- if after a (even partial) command there is '--help' show detailed help
	  for all the matching commands
	- if the command doesn't match show an error
	- finally, if a command matches, they return which command matched and
	  the arguments

	The function return 0 in case of help is requested; <0 in case
	of uncorrect command; >0 in case of matching commands
	argc, argv are the arg-counter and arg-vector (input)
	*nargs_ is the number of the arguments after the command (output)
	**cmd_  is the invoked command (output)
	***args_ are the arguments after the command

*/
static int parse_args(int argc, char **argv,
		      CommandFunction *func_,
		      int *nargs_, char **cmd_, char ***args_ )
{
	struct Command	*cp;
	struct Command	*matchcmd=0;
	char		*prgname = get_prgname(argv[0]);
	int		i=0, helprequested=0;

	if( argc < 2 || !strcmp(argv[1], "help") ||
		!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")){
		help(prgname);
		return 0;
	}

	for( cp = commands; cp->verb; cp++ )
		if( !cp->ncmds)
			cp->ncmds = split_command(cp->verb, &(cp->cmds));

	for( cp = commands; cp->verb; cp++ ){
		int     match;

		if( argc-1 < cp->ncmds )
			continue;
		for( match = 1, i = 0 ; i < cp->ncmds ; i++ ){
			char	*s1, *s2;
			s1 = cp->cmds[i];
			s2 = argv[i+1];

			for(s2 = cp->cmds[i], s1 = argv[i+1];
				*s1 == *s2 && *s1;
				s1++, s2++ ) ;
			if( *s1 ){
				match=0;
				break;
			}
		}

		/* If you understand why this code works ...
			you are a genious !! */
		if(argc>i+1 && !strcmp(argv[i+1],"--help")){
			if(!helprequested)
				printf("Usage:\n");
			print_help(prgname, cp, ADVANCED_HELP);
			helprequested=1;
			continue;
		}

		if(!match)
			continue;

		matchcmd = cp;
		*nargs_  = argc-matchcmd->ncmds-1;
		*cmd_ = matchcmd->verb;
		*args_ = argv+matchcmd->ncmds+1;
		*func_ = cp->func;

		break;
	}

	if(helprequested){
		printf("\n%s\n", MMC_VERSION);
		return 0;
	}

	if(!matchcmd){
		fprintf( stderr, "ERROR: unknown command '%s'\n",argv[1]);
		help(prgname);
		return -1;
	}

	if(check_ambiguity(matchcmd, argv))
		return -2;

	/* check the number of argument */
	if (matchcmd->nargs < 0 && matchcmd->nargs < -*nargs_ ){
		fprintf(stderr, "ERROR: '%s' requires minimum %d arg(s)\n",
			matchcmd->verb, -matchcmd->nargs);
			return -2;
	}
	if(matchcmd->nargs >= 0 && matchcmd->nargs != *nargs_ && matchcmd->nargs != 999){
		fprintf(stderr, "ERROR: '%s' requires %d arg(s)\n",
			matchcmd->verb, matchcmd->nargs);
			return -2;
	}
	
        if (prepare_args( nargs_, args_, prgname, matchcmd )){
                fprintf(stderr, "ERROR: not enough memory\\n");
		return -20;
        }


	return 1;
}
int main(int ac, char **av )
{
	char		*cmd=0, **args=0;
	int		nargs=0, r;
	CommandFunction func=0;

	r = parse_args(ac, av, &func, &nargs, &cmd, &args);
	if( r <= 0 ){
		/* error or no command to parse*/
		exit(-r);
	}

	exit(func(nargs, args));
}
