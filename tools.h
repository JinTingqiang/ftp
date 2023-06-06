#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdbool.h>

#define ERROR(func) printf("file:%s func:%s line:%d [%s]:%m\n",__FILE__,__func__,__LINE__,func);

//	从键盘获取指定长度的字符串
char* get_str(char* str,size_t hope_len);

//	输入指定长度密码
char* get_pass(char* str,size_t hope_len,bool is_show);

//	文件数据读写
void file_oi(int ofd,int ifd);

#endif//TOOLS_H
