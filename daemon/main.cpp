/*
 * main.cpp
 *  Created on: Oct 29, 2016
 *      Author: zhangyalei
 */

#include <signal.h>
#include <getopt.h>
#include "Aes.h"
#include "Compress.h"
#include "Struct_Tool.h"
#include "Daemon_Server.h"

struct option long_options[] = {
		{"struct_tool",		no_argument,		0,	's'},
		{"compress",		no_argument,		0,	'c'},
		{"decompress",		no_argument,		0,	'd'},
		{0, 0, 0, 0}
};

int parse_cmd_arguments(int argc, char *argv[]) {
	Struct_Tool tool;
	bool daemon = true;
	int c = 0;
	while ((c = getopt_long_only(argc, argv, "vdm:", long_options, NULL)) != -1) {
		//带参数时候，不启动daemon_server
		daemon = false;
		switch (c) {
		case 's': { //struct_tool
			tool.write_struct();
			break;
		}
		case 'c': { //compress
			folder_encrypt("js/");
			folder_comp("js/");
			break;
		}
		case 'd': { //decompress
			folder_decomp("js/");
			folder_decrypt("js/");
			break;
		}
		default: {
			break;
		}
		}
	}

	if (daemon) {
		tool.write_struct();
		DAEMON_SERVER->init(argc, argv);
		DAEMON_SERVER->start(argc, argv);
	}
	return 0;
}

static void sighandler(int sig_no) { exit(0); } /// for gprof need normal exit

int main(int argc, char *argv[]) {
	signal(SIGPIPE, SIG_IGN);
	signal(SIGUSR1, sighandler);

	parse_cmd_arguments(argc, argv);
	return 0;
}
