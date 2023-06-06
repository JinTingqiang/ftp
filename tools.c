#include <getch.h>
#include <unistd.h>
#include "tools.h"

//	从键盘获取指定长度的字符串
char* get_str(char* str,size_t hope_len)
{
	if(NULL == str || 0 == hope_len) return NULL;

	size_t index = 0;
	while(index < hope_len-1)
	{
		int key_val = getch();
		if(10 == key_val) break;
		if(127 == key_val)
		{
			if(index) 
			{
				index--;
				printf("\b \b");
			}
			continue;
		}
		str[index++] = key_val;
		printf("%c",key_val);
	}
	str[index] = '\0';
	printf("\n");
	stdin->_IO_read_ptr = stdin->_IO_read_end;
	return str;
}

//	输入指定长度密码
char* get_pass(char* str,size_t hope_len,bool is_show)
{
	if(NULL == str || 0 == hope_len) return NULL;

	size_t index = 0;
	while(index < hope_len-1)
	{
		int key_val = getch();
		if(10 == key_val) break;
		if(127 == key_val)
		{
			if(index) 
			{
				index--;
				printf("\b \b");
			}
			continue;
		}
		str[index++] = key_val;
		if(is_show)	printf("%c",key_val);
		else printf("*");
	}
	str[index] = '\0';
	printf("\n");
	stdin->_IO_read_ptr = stdin->_IO_read_end;
	return str;
	
}

//	文件数据读写
void file_oi(int ofd,int ifd)
{
	char buf[4096] = {};
	int ret = 0;
	while((ret = read(ofd,buf,sizeof(buf))))
	{
		write(ifd,buf,ret);	
	}
}




