/*
 * main.cpp
 *  Created on: Sep 19, 2016
 *      Author: zhangyalei
 */

#include <signal.h>
#include "Time_Value.h"
#include "Daemon_Manager.h"

static void sighandler(int sig_no) { exit(0); } /// for gprof need normal exit

int main(int argc, char *argv[]) {
	srand(Time_Value::gettimeofday().sec() + Time_Value::gettimeofday().usec());
	srandom(Time_Value::gettimeofday().sec() + Time_Value::gettimeofday().usec());
	signal(SIGPIPE, SIG_IGN);
	signal(SIGUSR1, sighandler);

	DAEMON_MANAGER->init(argc, argv);
	DAEMON_MANAGER->start(argc, argv);

	return 0;
}
