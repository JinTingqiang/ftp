#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ftp_client.h"
#include "tools.h"

int main(int argc,const char* argv[])
{
	//	创建FTP对象
	FTPClient* ftp = create_FTPClient();
	if(NULL == ftp)	return EXIT_FAILURE;

	if(2 == argc)
	{
		//	连接ftp	
		if(connect_FTPClient(ftp,argv[1],21)) return EXIT_FAILURE;
	}
	else if(3 == argc)
	{
		//	连接ftp	
		if(connect_FTPClient(ftp,argv[1],atoi(argv[2])))
			return EXIT_FAILURE;
	}
	else
	{
		printf("User: ./ftp xxx.xxx.xxx.xxx [port]\n");
		return EXIT_SUCCESS;
	}
	
	//	发送用户名
	char user[20] = {};
	printf("Name:");
	get_str(user,20);
	user_FTPClient(ftp,user);

	//	发送密码
	char pass[20] = {};
	printf("Password:");
	get_pass(pass,20,false);
	pass_FTPClient(ftp,pass);

	char cmd[10+PATH_MAX] = {};
	size_t cmd_size = sizeof(cmd);

	pwd_FTPClient(ftp);
	//	循环输入命令、发送命令
	for(;;)
	{
		printf("ftp>");
		get_str(cmd,cmd_size);
		if(0 == strncmp(cmd,"pwd",3))
			pwd_FTPClient(ftp);
		else if(0 == strncmp(cmd,"cd ",3))
			cd_FTPClient(ftp,cmd+3);
		else if(0 == strncmp(cmd,"mkdir ",6))
			mkdir_FTPClient(ftp,cmd+6);
		else if(0 == strncmp(cmd,"rmdir ",6))
			rmdir_FTPClient(ftp,cmd+6);
		else if(0 == strncmp(cmd,"bye",3))
			bye_FTPClient(ftp);
		else if('!' == cmd[0])
			system(cmd+1);
		else if(0 == strncmp(cmd,"ls",2))
			ls_FTPClient(ftp);
		else if(0 == strncmp(cmd,"get ",4))
			get_FTPClient(ftp,cmd+4);
		else if(0 == strncmp(cmd,"put ",4))
			put_FTPClient(ftp,cmd+4);
		else
			printf("指令未定义!\n");
	}
}


