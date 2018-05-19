/*
 * Proc_Info.cpp
 *
 *  Created on: Nov 28,2016
 *      Author: zhangyalei
 */

#include "Proc_Info.h"

void str_ltrim(char*str, char ch)
{
	int len = (int)strlen(str);
	int pos = -1;
	for (int i = 0; i < len; ++i)
	{
		if (str[i] == ch)
		{
			pos = i;
		}
		else
		{
			break;
		}
	}
	if (pos >= 0)
	{
		for (int i = 0; i < (int)(len - pos - 1); ++i)
		{
			str[i] = str[i + pos + 1];
		}
		str[len - pos - 2] = 0;
	}
}

void str_rtrim(char*str, char ch)
{
	int pos = 0;
	int len = (int)strlen(str);

	for (int i = 0; i < len; ++i)
	{
		if (str[i] != ch)
		{
			pos = i;
		}
	}
	str[pos + 1] = 0;
}

void str_trim(char* str, char ch)
{
	str_ltrim(str, ch);
	str_rtrim(str, ch);
}


void get_proc_cpuinfo(proc_cpuinfo_t* info)
{
	char filename[260] = {0};
	sprintf(filename, "/proc/cpuinfo");

	FILE* fp = fopen(filename, "r");
	if (fp)
	{
		char* sep = 0;
		std::string strName = "";
		std::string strValue = "";
		int cpu_count = 0;
		while(!feof(fp))
		{
			char buff[16*1024] = {0};
			char name[16*1024] = {0};
			char value[16*1024] = {0};

			fgets(buff, sizeof(buff), fp);

			sep = strstr(buff, ":");
			if (sep)
			{
				strncpy(name, buff, sep - buff);
				str_trim(name, ' ');
				str_trim(name, '\t');
				strcpy(value, sep + 1);
				str_trim(value, ' ');
				str_trim(value, '\t');

				strName = name;
				strValue = value;
				if (strName.length() > 0)
				{
					if (strName == "processor")
					{
						++cpu_count;
					}
					if (cpu_count > 0)
					{
						(*info)[cpu_count-1][strName] = strValue;
					}
				}
			}
		}
#if DEBUG_PROC_INFO
		int n = 0;
		printf("[proc_cpuinfo_t]\n");
		for (proc_cpuinfo_t::iterator i = (*info).begin(); i != (*info).end(); ++i)
		{
			printf("[processor %u]\n", n++);
			proc_info_t processor_info = i->second;
			for (proc_info_t::iterator j = processor_info.begin(); j != processor_info.end(); ++j)
			{
				printf("%s=%s\n", j->first.c_str(), j->second.c_str());
			}
			printf("\n");
		}
		printf("\n");
#endif
		fclose(fp);
	}
}


void get_proc_pid_status(uint32_t pid, proc_info_t* info)
{
	char filename[260] = {0};
	sprintf(filename, "/proc/%u/status", pid);

	FILE* fp = fopen(filename, "r");
	if (fp)
	{
		char* sep = 0;
		std::string strName = "";
		std::string strValue = "";
		while(!feof(fp))
		{
			char buff[16*1024] = {0};
			char name[16*1024] = {0};
			char value[16*1024] = {0};

			fgets(buff, sizeof(buff), fp);

			sep = strstr(buff, ":");
			if (sep)
			{
				strncpy(name, buff, sep - buff);
				str_trim(name, ' ');
				str_trim(name, '\t');
				strcpy(value, sep + 1);
				str_trim(value, ' ');
				str_trim(value, '\t');

				strName = name;
				strValue = value;
				if (strName.length() > 0)
				{
					(*info)[strName] = strValue;
				}
			}
		}
#if DEBUG_PROC_INFO
		printf("[proc_cpuinfo_t]\n");
		for (proc_info_t::iterator j = (*info).begin(); j != (*info).end(); ++j)
		{
			printf("%s=%s\n", j->first.c_str(), j->second.c_str());
		}
		printf("\n");
#endif
		fclose(fp);
	}
}

void get_proc_stat_cpu(proc_stat_cpu_t* info)
{
	char filename[260] = {0};
	sprintf(filename, "/proc/stat");

	FILE* fp = fopen(filename, "r");
	if (fp)
	{
		char buff[128] = {0};
		fgets(buff, sizeof(buff), fp);

		sscanf(buff, "%s %u %u %u %u %u %u %u", 
			info->cpu, 
			&info->user, 
			&info->nice,
			&info->system,
			&info->idle,
			&info->iowait,
			&info->irq,
			&info->softirq);
#if DEBUG_PROC_INFO
		printf("%s ", info->cpu);
		printf("%u ", info->user);
		printf("%u ", info->nice);
		printf("%u ", info->system);
		printf("%u ", info->idle);
		printf("%u ", info->iowait);
		printf("%u ", info->irq);
		printf("%u ", info->softirq);
		printf("\n");
#endif
		fclose(fp);
	}
}

void get_proc_pid_statm(uint32_t pid, proc_pid_statm_t* info)
{
	char filename[260] = {0};
	sprintf(filename, "/proc/%u/statm", pid);

	FILE* fp = fopen(filename, "r");
	if (fp)
	{
		char buff[1024] = {0};
		size_t size = fread(buff, 1, sizeof(buff), fp);
		if (size)
		{
			sscanf(buff, "%u %u %u %u %u %u %u", 
				&info->size, 
				&info->resident, 
				&info->share, 
				&info->text, 
				&info->lib, 
				&info->data, 
				&info->dt);

#if DEBUG_PROC_INFO
			for (int i = 0; i < 7; ++i)
			{
				printf("%u ", ((uint32_t*)info)[i]);
			}
			printf("\n");
#endif
		}
		fclose(fp);
	}
}

void get_proc_pid_stat(uint32_t pid, proc_pid_stat_t* info)
{
	char filename[260] = {0};
	sprintf(filename, "/proc/%u/stat", pid);

	FILE* fp = fopen(filename, "r");
	if (fp)
	{
		char buff[1024] = {0};
		fgets(buff, sizeof(buff), fp);

		sscanf(buff, "%d %s \
					 %c %d %d %d %d  \
					 %d %u %u %u %u \
					 %u %u %u %d %d \
					 %d %d %d %d %u \
					 %u %d %u %u %u \
					 %u %u %u %u %u \
					 %u %u %u %u %u \
					 %d %d %u %u %lu",

					 &info->pid, 
					 info->comm, 

					 &info->state, 
					 &info->ppid, 
					 &info->pgrp, 
					 &info->session, 
					 &info->tty_nr,

					 &info->tpgid,
					 &info->flags,
					 &info->minflt,
					 &info->cminflt,
					 &info->majflt,

					 &info->cmajflt,
					 &info->utime,
					 &info->stime,
					 &info->cutime,
					 &info->cstime,

					 &info->priority,
					 &info->nice,
					 &info->num_threads,
					 &info->itrealvalue,
					 &info->starttime,

					 &info->vsize,
					 &info->rss,
					 &info->rlim,
					 &info->startcode,
					 &info->endcode,

					 &info->startstack,
					 &info->kstkesp,
					 &info->kstkeip,
					 &info->signal,
					 &info->blocked,

					 &info->sigignore,
					 &info->sigcatch,
					 &info->wchan,
					 &info->nswap,
					 &info->cnswap,

					 &info->exit_signal,
					 &info->processor,
					 &info->rt_priority,
					 &info->policy,
					 &info->delayacct_blkio_ticks);

#if DEBUG_PROC_INFO
		printf("%s\t=\t%d\n", GET_NAME(info->pid),			info->pid);
		printf("%s\t=\t%s\n", GET_NAME(info->comm),			info->comm);

		printf("%s\t=\t%c\n",  GET_NAME(info->state), 		info->state);
		printf("%s\t=\t%d\n",  GET_NAME(info->ppid), 		info->ppid);
		printf("%s\t=\t%d\n",  GET_NAME(info->pgrp), 		info->pgrp);
		printf("%s\t=\t%d\n",  GET_NAME(info->session), 	info->session);
		printf("%s\t=\t%d\n",  GET_NAME(info->tty_nr), 		info->tty_nr);

		printf("%s\t=\t%d\n",  GET_NAME(info->tpgid), 		info->tpgid);
		printf("%s\t=\t%u\n", GET_NAME(info->flags), 		info->flags);
		printf("%s\t=\t%u\n", GET_NAME(info->minflt), 		info->minflt);
		printf("%s\t=\t%u\n", GET_NAME(info->cminflt), 	info->cminflt);
		printf("%s\t=\t%u\n", GET_NAME(info->majflt), 		info->majflt);

		printf("%s\t=\t%u\n", GET_NAME(info->cmajflt), 	info->cmajflt);
		printf("%s\t=\t%u\n", GET_NAME(info->utime), 		info->utime);
		printf("%s\t=\t%u\n", GET_NAME(info->stime), 		info->stime);
		printf("%s\t=\t%d\n", GET_NAME(info->cutime), 		info->cutime);
		printf("%s\t=\t%d\n", GET_NAME(info->cstime), 		info->cstime);

		printf("%s\t=\t%d\n", GET_NAME(info->priority),	info->priority);
		printf("%s\t=\t%d\n", GET_NAME(info->nice), 		info->nice);
		printf("%s\t=\t%d\n", GET_NAME(info->num_threads), info->num_threads);
		printf("%s\t=\t%d\n", GET_NAME(info->itrealvalue), info->itrealvalue);
		printf("%s\t=\t%d\n", GET_NAME(info->starttime), 	info->starttime);

		printf("%s\t=\t%u\n", GET_NAME(info->vsize), 		info->vsize);
		printf("%s\t=\t%d\n", GET_NAME(info->rss), 		info->rss);
		printf("%s\t=\t%u\n", GET_NAME(info->rlim), 		info->rlim);
		printf("%s\t=\t%u\n", GET_NAME(info->startcode), 	info->startcode);
		printf("%s\t=\t%u\n", GET_NAME(info->endcode), 	info->endcode);

		printf("%s\t=\t%u\n", GET_NAME(info->startstack), 	info->startstack);
		printf("%s\t=\t%u\n", GET_NAME(info->kstkesp), 	info->kstkesp);
		printf("%s\t=\t%u\n", GET_NAME(info->kstkeip), 	info->kstkeip);
		printf("%s\t=\t%u\n", GET_NAME(info->signal), 		info->signal);
		printf("%s\t=\t%u\n", GET_NAME(info->blocked), 	info->blocked);

		printf("%s\t=\t%u\n", GET_NAME(info->sigignore), 	info->sigignore);
		printf("%s\t=\t%u\n", GET_NAME(info->sigcatch), 	info->sigcatch);
		printf("%s\t=\t%u\n", GET_NAME(info->wchan), 		info->wchan);
		printf("%s\t=\t%u\n", GET_NAME(info->nswap), 		info->nswap);
		printf("%s\t=\t%u\n", GET_NAME(info->cnswap), 		info->cnswap);

		printf("%s\t=\t%d\n",  GET_NAME(info->exit_signal), info->exit_signal);
		printf("%s\t=\t%d\n",  GET_NAME(info->processor), 	info->processor);
		printf("%s\t=\t%u\n", GET_NAME(info->rt_priority), info->rt_priority);
		printf("%s\t=\t%u\n", GET_NAME(info->policy), 		info->policy);
		printf("%s\t=\t%lu\n",GET_NAME(info->delayacct_blkio_ticks), 	info->delayacct_blkio_ticks);
#endif
		fclose(fp);
	}
}

uint32_t get_cpu_processor_num(void)
{
	proc_cpuinfo_t cpu_info;
	get_proc_cpuinfo(&cpu_info);
	return (uint32_t)cpu_info.size();
}

uint32_t get_total_cpu_used(void)
{
	proc_stat_cpu_t cpu;
	get_proc_stat_cpu(&cpu);
	uint32_t total_cpu_time = (cpu.user + cpu.nice + cpu.system + cpu.idle + cpu.iowait + cpu.irq + cpu.softirq);
	return total_cpu_time;
}

uint32_t get_pid_cpu_used(uint32_t pid)
{
	proc_pid_stat_t pid_info;
	get_proc_pid_stat(pid, &pid_info);
	uint32_t pid_cpu_time = pid_info.utime + pid_info.stime + pid_info.cutime + pid_info.cstime;
	return pid_cpu_time;
}

float get_cpu_used_percent(uint32_t pid)
{
	static uint32_t pid_used_old = 0;
	static uint32_t total_used_old = 0;

	uint32_t pid_used_new = 0;
	uint32_t total_used_new = 0;

	uint8_t cpu_num = get_cpu_processor_num();
	pid_used_new = get_pid_cpu_used(pid);
	total_used_new = get_total_cpu_used();

	uint32_t pid_diff = pid_used_new - pid_used_old;
	uint32_t total_diff = total_used_new - total_used_old;
	float used_percent = (100.0f * pid_diff) / (float)total_diff;
	if (total_diff)
	{
		used_percent = (100.0f * pid_diff) / (float)total_diff;
	}
	else
	{
		used_percent = (pid_diff > total_diff) ? 100.0f : 0.0f;
	}
	used_percent *= cpu_num;
	//printf("[%u]:CPU(%u)=%u/%u, %.2f\n", pid, cpu_num, pid_diff, total_diff, used_percent);

	pid_used_old = pid_used_new;
	total_used_old = total_used_new;

	return used_percent;
}

uint32_t get_total_mem_size(void)
{
	proc_pid_stat_t pid_info;
	get_proc_pid_stat(getpid(), &pid_info);
	uint32_t total_mem = pid_info.rlim;
	return total_mem;
}

proc_pid_mem_t get_pid_mem_used(uint32_t pid)
{
#if 0
	//鉴于数据和真实情况差距太大，不予采用
	proc_pid_statm_t pid_mem;
	get_proc_pid_statm(pid, &pid_mem);
	printf("[%u](statm):VSZ=%u, RSS=%u, share=%u, text=%u, data=%u\n", pid, pid_mem.size, pid_mem.resident, pid_mem.share, pid_mem.text, pid_mem.data);
	
	proc_pid_stat_t pid_info;
	get_proc_pid_stat(pid, &pid_info);
	printf("[%u](stat):VSZ=%u, RSS=%u, rlim=%u\n", pid, pid_info.vsize, pid_info.rss, pid_info.rlim);
#endif
	proc_info_t info;
	get_proc_pid_status(pid, &info);
	//printf("[%u](status):VSZ=%s, RSS=%s, share=%s, text=%s, data=%s\n", pid, info["VmSize"].c_str(), info["VmRSS"].c_str(), info["VmStk"].c_str(), info["VmExe"].c_str(), info["VmData"].c_str());
	
	proc_pid_mem_t pid_mem;
	sscanf(info["VmSize"].c_str(), " %lu ", &pid_mem.VmSize);
	sscanf(info["VmRSS"].c_str(), " %lu ", &pid_mem.VmRSS);
	sscanf(info["VmStk"].c_str(), " %lu ", &pid_mem.VmStk);
	sscanf(info["VmExe"].c_str(), " %lu ", &pid_mem.VmExe);
	sscanf(info["VmData"].c_str(), " %lu ", &pid_mem.VmData);
	return pid_mem;
}

float get_mem_used_percent(uint32_t pid)
{
	uint32_t total_size = get_total_mem_size();
	proc_pid_mem_t pid_used = get_pid_mem_used(pid);
	uint64_t total_used = (pid_used.VmSize + pid_used.VmRSS + pid_used.VmStk + pid_used.VmExe + pid_used.VmData) * 1024;
	float used_percent = (100.0f * total_used) / (1.0f * total_size);
	//printf("[%u]:MEM=%u/%u, %.2f\n", pid, pid_used, total_used, used_percent);
	return used_percent;
}

void ps(void)
{
	char buff[128] = {0};
	sprintf(buff, "ps u -p %u", getpid());
	system(buff);
}