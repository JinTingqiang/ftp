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
#include <getch.h>
#include "ftp_client.h"
#include "tools.h"

//	连接
static int _connect(int sockfd,const char* ip,uint16_t port)
{
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	if(connect(sockfd,(SP)&addr,sizeof(addr)))
	{
		ERROR("connect");
		return -1;
	}
	return EXIT_SUCCESS;
}

//	接收命令结果并判断
static int recv_status(FTPClient* ftp,int status,bool die)
{
	size_t size = recv(ftp->cli_sock,ftp->buf,BUF_SIZE,0);	
	if(0 >= size)
	{
		printf("与FTP服务器断开连接...\n");	
		destroy_FTPClient(ftp);
		exit(EXIT_FAILURE);
	}
	
	ftp->buf[size] = '\0';
	printf("%s",ftp->buf);

	int ret_status = 0;
	sscanf(ftp->buf,"%d",&ret_status);

	if(ret_status != status && die)
	{
		destroy_FTPClient(ftp);
		exit(EXIT_FAILURE);
	}
	
	return ret_status == status ? EXIT_SUCCESS:EXIT_FAILURE;
}

//	发送命令
static void send_cmd(FTPClient* ftp)
{
	int ret = send(ftp->cli_sock,ftp->buf,strlen(ftp->buf),0);	
	if(0 > ret)
	{
		printf("与FTP服务器断开连接...\n");	
		destroy_FTPClient(ftp);
		ERROR("send");
		exit(EXIT_FAILURE);
	}
}

//	开启PASV
static int pasv_FTPClient(FTPClient* ftp)
{
	sprintf(ftp->buf,"PASV\n");
	send_cmd(ftp);
	if(recv_status(ftp,227,false)) return EXIT_FAILURE;

	uint8_t ip1,ip2,ip3,ip4;
	char ip[16] = {};
	uint16_t port1,port2,port;

	sscanf(ftp->buf,"227 Entering Passive Mode (%hhu,%hhu,%hhu,%hhu,%hu,%hu)",&ip1,&ip2,&ip3,&ip4,&port1,&port2);

	sprintf(ip,"%hhu.%hhu.%hhu.%hhu",ip1,ip2,ip3,ip4);
	port = port1*256+port2;

	ftp->cli_pasv = socket(AF_INET,SOCK_STREAM,0);
	if(0 > ftp->cli_pasv)
	{
		ERROR("socket");
		return EXIT_FAILURE;
	}

	return _connect(ftp->cli_pasv,ip,port);
}


//	创建FTP客户端对象
FTPClient* create_FTPClient(void)
{
	FTPClient* ftp = malloc(sizeof(FTPClient));	

	ftp->cli_sock = socket(AF_INET,SOCK_STREAM,0);
	if(0 > ftp->cli_sock)
	{
		free(ftp);
		ERROR("socket");
		return NULL;
	}

	ftp->buf = malloc(BUF_SIZE);
	return ftp;
}

//	销毁FTP客户端对象
void destroy_FTPClient(FTPClient* ftp)
{
	close(ftp->cli_sock);
	free(ftp->buf);
	free(ftp);
}

//	登录连接FTP服务器
int connect_FTPClient(FTPClient* ftp,const char* ip,uint16_t port)
{
	if(_connect(ftp->cli_sock,ip,port))
	{
		return EXIT_FAILURE;	
	}
	
	recv_status(ftp,220,true);

	return EXIT_SUCCESS;	
}

//	向服务器发送用户名
void user_FTPClient(FTPClient* ftp,const char* user)
{
	sprintf(ftp->buf,"USER %s\n",user);	
	send_cmd(ftp);
	recv_status(ftp,331,true);
}

//	向服务器发送密码
void pass_FTPClient(FTPClient* ftp,const char* pass)
{
	sprintf(ftp->buf,"PASS %s\n",pass);	
	send_cmd(ftp);
	recv_status(ftp,230,true);
}

//	pwd
void pwd_FTPClient(FTPClient* ftp)
{
	sprintf(ftp->buf,"PWD\n");	
	send_cmd(ftp);
	recv_status(ftp,257,false);
	
	//	记录服务器的当前工作路径
	sscanf(ftp->buf,"%*d \"%s",ftp->server_path);
	*strchr(ftp->server_path,'\"') = '\0';
//	printf("server_path:%s\n",ftp->server_path);
}

//	cd
void cd_FTPClient(FTPClient* ftp,const char* path)
{
	sprintf(ftp->buf,"CWD %s\n",path);	
	send_cmd(ftp);
	recv_status(ftp,250,false);

	pwd_FTPClient(ftp);
}

//	mkdir
void mkdir_FTPClient(FTPClient* ftp,const char* dir)
{
	sprintf(ftp->buf,"MKD %s\n",dir);	
	send_cmd(ftp);
	recv_status(ftp,257,false);
}

//	rmdir
void rmdir_FTPClient(FTPClient* ftp,const char* dir)
{
	sprintf(ftp->buf,"RMD %s\n",dir);	
	send_cmd(ftp);
	recv_status(ftp,250,false);
}

//	bye
void bye_FTPClient(FTPClient* ftp)
{
	sprintf(ftp->buf,"QUIT\n");	
	send_cmd(ftp);
	recv_status(ftp,221,false);
	destroy_FTPClient(ftp);
	exit(EXIT_SUCCESS);
}

//	ls
void ls_FTPClient(FTPClient* ftp)
{
	if(pasv_FTPClient(ftp)) return;

	sprintf(ftp->buf,"LIST -al\n");	
	send_cmd(ftp);
	if(recv_status(ftp,150,false))
	{
		close(ftp->cli_pasv);
		return;
	}

	//	接收数据
	file_oi(ftp->cli_pasv,STDOUT_FILENO);	//	写入标准输出文件
	close(ftp->cli_pasv);

	//	接收执行结果
	recv_status(ftp,226,false);
}

//	get
void get_FTPClient(FTPClient* ftp,const char* file)
{
	sprintf(ftp->buf,"SIZE %s\n",file);	
	send_cmd(ftp);
	if(recv_status(ftp,213,false))
	{
		printf("待下载%s文件不存在\n",file);
		return;
	}
	
	int file_fd = open(file,O_WRONLY|O_CREAT,0644);
	if(0 > file_fd)
	{
		printf("当前目录没有写权限,下载失败\n");
		return;
	}

	//	开启PASV
	if(pasv_FTPClient(ftp))
	{
		close(file_fd);
		return;
	}

	//	开始下载
	sprintf(ftp->buf,"RETR %s\n",file);	
	send_cmd(ftp);
	if(recv_status(ftp,150,false))
	{
		close(file_fd);
		close(ftp->cli_pasv);
		return;
	}

	file_oi(ftp->cli_pasv,file_fd);	

	close(file_fd);
	close(ftp->cli_pasv);
	recv_status(ftp,226,false);
}

//	put
/*
void put_FTPClient(FTPClient* ftp,const char* file)
{
	int file_fd = open(file,O_RDONLY);
	if(0 > file_fd)
	{
		printf("%s文件不存在,上传失败\n",file);
		return;
	}

	//	开启PASV
	if(pasv_FTPClient(ftp))
	{
		close(file_fd);
		return;
	}

	//	开始上传
	sprintf(ftp->buf,"STOR %s\n",file);	
	send_cmd(ftp);
	if(recv_status(ftp,150,false))
	{
		close(file_fd);
		close(ftp->cli_pasv);
		return;
	}

//	file_oi(file_fd,ftp->cli_pasv);
	
	//	零拷贝
	//	获取文件大小
	struct stat stat_buf;
	fstat(file_fd,&stat_buf);

	//	sendfile都是内核拷贝，没有经过多次用户态、内核态的切换和拷贝，拷贝次数大大减少，因此称为零拷贝技术
	//	ssize_t  sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
	//	out_fd 用于写操作的打开的文件描述符
	//	in_fd 用于读操作的打开的文件描述符
	//	offset 用于随机读写
	//	count 要拷贝的文件字节数
	sendfile(ftp->cli_pasv,file_fd,NULL,stat_buf.st_size);

	close(file_fd);
	close(ftp->cli_pasv);
	recv_status(ftp,226,false);
}
*/

//	获取文件的最后修改时间
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

//	获取文件大小
size_t file_size(int filefd)
{
	struct stat stat_buf;
	if(fstat(filefd,&stat_buf)) return -1;
	
	return stat_buf.st_size;
}

//	断点续传的上传命令
void put_FTPClient(FTPClient* ftp,const char* file)
{
	//	如果本地文件不存在则结束
	int file_fd = open(file,O_RDONLY);
	if(0 > file_fd)
	{
		printf("%s文件不存在，上传失败\n",file);
		return;
	}

	//	服务器文件最后修改时间
	char sflie_mdtm[15] = {};
	//	客户端文件最后修改时间
	char cflie_mdtm[15] = {};
	//	获取客户端文件最后修改时间
	get_file_mdtm(file,cflie_mdtm);

	sprintf(ftp->buf,"SIZE %s\n",file);
	send_cmd(ftp);
	//	检查服务器是否存在同名文件
	if(!recv_status(ftp,213,false))
	{
		//	记录服务器文件大小
		size_t sfile_size = 0;
		sscanf(ftp->buf,"213 %u",&sfile_size);

		//	是：检查该文件最后修改时间，是否是同一个文件
		sprintf(ftp->buf,"MDTM %s\n",file);
		send_cmd(ftp);
		recv_status(ftp,213,false);
		sscanf(ftp->buf,"%*d %s",sflie_mdtm);
		printf("sflie_mdtm:%s\n",sflie_mdtm);
		if(0 == strcmp(sflie_mdtm,cflie_mdtm))
		{
			//	获取客户端文件大小
			size_t cfile_size = file_size(file_fd);

			//	是：根据文件大小决定是否断点续传
			if(sfile_size == cfile_size)
			{
				//大小相同：上传结束
				printf("上传结束\n");
				close(file_fd);
				return;
			}
			else
			{
				//大小不相同：断点续传
				printf("开始断点续传\n");
				sprintf(ftp->buf,"REST %u\n",sfile_size);
				send_cmd(ftp);
				recv_status(ftp,350,false);
				lseek(file_fd,sfile_size,SEEK_SET);
			}
		}
		else
		{
			//	否：不是同一个文件，询问是否覆盖
			printf("服务器存在同名文件，是否覆盖(y/n)?");
			int cmd = getch();
			printf("%c\n",cmd);
			if('y' != cmd)
			{
				printf("终止上传\n");
				close(file_fd);
				return;
			}
		}
	}
	//	不是：正常上传
	//	开启PASV
	if(pasv_FTPClient(ftp))
	{
		close(file_fd);
		return;
	}

	//	开始上传
	sprintf(ftp->buf,"STOR %s\n",file);	
	send_cmd(ftp);
	if(recv_status(ftp,150,false))
	{
		close(file_fd);
		close(ftp->cli_pasv);
		return;
	}
	file_oi(file_fd,ftp->cli_pasv);

	close(file_fd);
	close(ftp->cli_pasv);
	recv_status(ftp,226,false);

	//	上传文件的最后修改时间
	sprintf(ftp->buf,"MDTM %s %s/%s\n",cflie_mdtm,ftp->server_path,file);
	printf("-----%s\n",ftp->buf);
	send_cmd(ftp);
	recv_status(ftp,213,false);
}














