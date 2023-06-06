#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>

int get_file_mdtm(const char* file,char* mdtm)
{
	struct stat stat_buf;
	if(stat(file,&stat_buf)) return -1;
	
	//	转换得到最后修改时间结构体
	struct tm* t_m = localtime(&stat_buf.st_mtim.tv_sec);
	if(NULL == t_m)	return -1;

	sprintf(mdtm,"%04d%02d%02d%02d%02d%02d",
		t_m->tm_year+1900,
		t_m->tm_mon+1,
		t_m->tm_mday,
		t_m->tm_hour,
		t_m->tm_min,
		t_m->tm_sec);
	return 0;
}
int main(int argc,const char* argv[])
{
	char dtm[15] = {};
	get_file_mdtm("main.c",dtm);
	printf("%s\n",dtm);
}
