#include "stm32f10x.h"                  // Device header
#include "4G.h"
#include "Delay.h"
#include <string.h>
#include "usart.h"
#include "time.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "Flash.h"

//#define  SOIL
#define  WEATHER


#define pagebyte 1024
#define MAX_FILE_LEN  48
#define	MAX_FILE_NUM	10

char *strx;

uint8_t readpage = 0; //读取http信息页数
int  errcount = 0;	//失败次数 防止死循环
char ATSTR[BUFLEN];	//组建AT命令的函数

int len_buf212;   //缓冲数据长度


typedef enum {
	Web_OK,      // 网络连接OK 
	Web_ERROR		 // 网络连接错误
} WebState;    // 网络连接状态

WebState  Web = Web_OK;

// 解析状态
typedef enum {
	STATE_IDLE,
	STATE_READING_FILENAME,
	STATE_CHECK_END_TAG
}ParseState;

ParseState state = STATE_IDLE;

// 存储结果
typedef struct {
	char filenames[MAX_FILE_NUM][MAX_FILE_LEN];
	uint8_t count;
}BinFileLIST;

char filename_buffer[MAX_FILE_LEN];
uint8_t filename_idx = 0;
BinFileLIST result;


uint8_t HttpVersion[32]={0};          // 设备版本  起始地址0x300   长度正常为6-10
uint8_t AdreeMN[32]={0};							//设备MN码，用于识别版本使用

/*****************************************************
清空状态机解析器函数
*****************************************************/
void bin_parser_reset(void) {
    state = STATE_IDLE;
    filename_idx = 0;
    memset(&result, 0, sizeof(result));
}

/*****************************************************
状态机解析器的结果获取函数，用于返回已解析到的文件名列表
*****************************************************/
const BinFileLIST* bin_parser_get_result(void) {
    return &result;
}

/*****************************************************
核心函数：状态机搜索文件名    本地服务器测试
*****************************************************/
/*
void bin_parser_feed_byte(char c) {
    static const char *target = "value=\"";
    static uint8_t match_pos = 0;
    const char * CMP_MN = (const char*)AdreeMN;
		//const char * CMP_MN = "test";
    switch(state) {
        case STATE_IDLE:
            // 逐字符匹配 "value=""
            if (c == target[match_pos]) {
                match_pos++;
                if (match_pos == strlen(target)) {
                    state = STATE_READING_FILENAME;
                    filename_idx = 0;
                    match_pos = 0;
                }
            } else {
                match_pos = 0;
            }
            break;
            
        case STATE_READING_FILENAME:
            // 读取文件名直到引号
            if (c == '\"') {
                filename_buffer[filename_idx] = '\0';
                // 检查是否为.bin文件
							 // 检查是否为.bin文件
                if (filename_idx > (strlen(CMP_MN)+4) &&   strncmp(filename_buffer, CMP_MN, strlen(CMP_MN)) == 0  &&
                    strcmp(&filename_buffer[filename_idx - 4], ".bin") == 0 &&
                    result.count < MAX_FILE_NUM) {
                    // 避免重复
                    bool duplicate = false;
                    for (int i = 0; i < result.count; i++) {
                        if (strcmp(result.filenames[i], filename_buffer) == 0) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) {
                        strcpy(result.filenames[result.count], filename_buffer);
                        result.count++;
                    }
                }
                state = STATE_IDLE;
                filename_idx = 0;
            } else if (filename_idx < MAX_FILE_LEN - 1) {
                filename_buffer[filename_idx++] = c;
            }
            break;
    }
}
*/



/*****************************************************
核心函数：状态机搜索文件名    正式服务器
*****************************************************/

void bin_parser_feed_byte(char c) {
#ifdef SOIL
    static const char *start_tag = "<Key>soil/";      // 开始标签
	#endif
#ifdef WEATHER
		static const char *start_tag = "<Key>weather/";      // 开始标签
	#endif
	
    static const char *end_tag = "</Key>";       // 结束标签
    static uint8_t match_pos = 0;
    static bool in_key_tag = false;              // 标记是否在Key标签内
		const char * CMP_MN = (const char*)AdreeMN;
	
switch(state) {
        case STATE_IDLE:
            // 匹配 "<Key>" 起始标签
            if (c == start_tag[match_pos]) {
                match_pos++;
                if (match_pos == strlen(start_tag)) {
                    state = STATE_READING_FILENAME;
                    filename_idx = 0;
                    match_pos = 0;
                    in_key_tag = true;
                }
            } else {
                match_pos = 0;
            }
            break;
            
        case STATE_READING_FILENAME:
            // 读取文件名直到遇到 "<" (可能是</Key>的开始)
            if (c == '<' && in_key_tag) {
                // 检查是否是结束标签的开始
                match_pos = 1;  // 已经匹配了'<'
                state = STATE_CHECK_END_TAG;
            } else if (filename_idx < MAX_FILE_LEN - 1) {
                filename_buffer[filename_idx++] = c;
            }
            break;
            
        case STATE_CHECK_END_TAG:
            // 验证是否为 </Key> 结束标签
            if (c == '/' && match_pos == 1) {
                match_pos++;
            } else if (c == 'K' && match_pos == 2) {
                match_pos++;
            } else if (c == 'e' && match_pos == 3) {
                match_pos++;
            } else if (c == 'y' && match_pos == 4) {
                match_pos++;
            } else if (c == '>' && match_pos == 5) {
                // 完整匹配到 </Key>
                filename_buffer[filename_idx] = '\0';
                
                // 检查是否为.bin文件   //	strncmp(filename_buffer, "FNKJ", 4) == 0  &&
                if (filename_idx > (strlen(CMP_MN)+4) &&   					   strncmp(filename_buffer, CMP_MN, strlen(CMP_MN)) == 0  &&
                    strcmp(&filename_buffer[filename_idx - 4], ".bin") == 0 &&
                    result.count < MAX_FILE_NUM) {
                    
                    // 避免重复
                    bool duplicate = false;
                    for (int i = 0; i < result.count; i++) {
                        if (strcmp(result.filenames[i], filename_buffer) == 0) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) {
                        strcpy(result.filenames[result.count], filename_buffer);
                        result.count++;
                    }
                }
                
                // 重置状态
                state = STATE_IDLE;
                filename_idx = 0;
                match_pos = 0;
                in_key_tag = false;
            } else {
                // 不是结束标签，退回读取状态
                // 将已匹配的字符补回缓冲区
                if (match_pos > 0) {
                    filename_buffer[filename_idx++] = '<';
                    for (int i = 1; i < match_pos; i++) {
                        if (filename_idx < MAX_FILE_LEN - 1) {
                            filename_buffer[filename_idx++] = end_tag[i];
                        }
                    }
                }
                if (filename_idx < MAX_FILE_LEN - 1) {
                    filename_buffer[filename_idx++] = c;
                }
                state = STATE_READING_FILENAME;
                match_pos = 0;
            }
            break;
    }
}


/*****************************************************
批量解析函数：整段数据给到解析器
*****************************************************/
void bin_parser_feed_data(const char *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        bin_parser_feed_byte(data[i]);
    }
}

/*****************************************************
辅助函数：在指定长度内查找最后一个','
*****************************************************/
char* My_strrchr(const char *str, int c, size_t max_len) {
    const char *last = NULL;
    for (size_t i = 0; i < max_len && str[i] != '\0'; i++) {
        if (str[i] == c) {
            last = &str[i];
        }
    }
    return (char*)last;
}

/*****************************************************
清空模块反馈的信息
*****************************************************/
void Clear_Buffer(void)									
{
    printf(buf_uart1.buf);							// 打印缓冲区
    Delay_ms(100);												
    buf_uart1.index = 0;								// 清空缓冲区索引
    memset(buf_uart1.buf, 0, BUFLEN);		// 清空缓冲区
}

/*****************************************************
文件名比较函数
*****************************************************/
const char* find_latest_file_string(const BinFileLIST* files) {
    if (files->count == 0) return NULL;
    
    const char* latest = files->filenames[0];
    
    for (int i = 1; i < files->count; i++) {
        // 比较时间戳部分（去掉.bin后缀）
        // 例如 "251208.bin" → 比较 "251208"
        if (strcmp(files->filenames[i], latest) > 0) {
            latest = files->filenames[i];
        }
    }
    return latest;
}

/*****************************************************
初始化模块 和单片机连接，获取卡号和信号质量
*****************************************************/
void CSTX_4G_Init(void)
{
		IWDG_ReloadCounter();																							//喂狗
    Uart1_SendStr("AT+CPIN?\r\n");																		//查询SIM卡是否已准备就绪
    Delay_ms(1000);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"+CPIN: READY"); //准备就绪
    while(strx == NULL)
    {
        Clear_Buffer();
        Uart1_SendStr("AT+CPIN?\r\n");																//获取卡号，类似是否存在卡的意思，比较重要。
        Delay_ms(1000);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"+CPIN: READY"); //返回OK,说明卡是存在的
    }
		printf("****SIM  READY****\r\n");																					//打印提示		SIM准备就绪
	  Clear_Buffer();																										//清空缓冲
		
    Uart1_SendStr("AT+CSQ\r\n");																			//查询信号强度
    Delay_ms(500);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"+CSQ:"); 	//返回信号强度
    if(strx)	
       printf("***** CSQ:%s *****\r\n", buf_uart1.buf + 14);         			//打印信号强度
    Clear_Buffer();

    Uart1_SendStr("AT+CGREG?\r\n");																		//查询网络是否附着上
    Delay_ms(2000);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"+CGREG: 0,1"); //必须判断0,1或0,5才是正确的
		if(strx == NULL)
			strstr((const char*)buf_uart1.buf, (const char*)"+CGREG: 0,5"); //如果没有返回0,1则判断0,5
    while(strx == NULL)
    {
        Clear_Buffer();
        Uart1_SendStr("AT+CGREG?\r\n");																//获取激活状态
        Delay_ms(1000);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"+CGREG: 0,1"); 
				if(strx == NULL)
					strstr((const char*)buf_uart1.buf, (const char*)"+CGREG: 0,5"); //如果没有返回0,1则判断0,5
    }
    Clear_Buffer();
		
		Uart1_SendStr("AT+CGACT?\r\n");																		//查询网络是否附着上
    Delay_ms(500);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"+CGACT:"); //必须判断0,1或0,5才是正确的
		Clear_Buffer();
}

/*****************************************************	
关闭HTTP连接
*****************************************************/
void CSTX_HTTP_CLOSE(void)
{
		Uart1_SendStr("AT+HTTPTERM\r\n");	
		Delay_ms(100);
    Clear_Buffer();										 //清空缓冲区	
}


/*****************************************************	
建立http连接
*****************************************************/
bool CSTX_HTTP_GetFileList(void) {
    //CSTX_HTTP_CLOSE();
    
    // HTTP初始化
    Uart1_SendStr("AT+HTTPINIT\r\n");
    Delay_ms(1000);
    strx = strstr((const char*)buf_uart1.buf, "OK");
    while(strx == NULL) {
        Clear_Buffer();
        Uart1_SendStr("AT+HTTPINIT\r\n");
        Delay_ms(1000);
        strx = strstr((const char*)buf_uart1.buf, "OK");
    }
    Clear_Buffer();
    
    // 设置URL
		memset(ATSTR,0,sizeof(ATSTR));
    //测试服务器
		//sprintf(ATSTR, "AT+HTTPPARA=\"URL\",\"http://1oi1711bc9202.vicp.fun/weather/\"\r\n");						
		//正式服务器
		sprintf(ATSTR, "AT+HTTPPARA=\"URL\",\"http://39.91.167.37:9000/hardwaseconfig/\"\r\n");
    Uart1_SendStr(ATSTR);
    Delay_ms(500);
    strx = strstr((const char*)buf_uart1.buf, "OK");
    while(strx == NULL) {
        Clear_Buffer();
        Uart1_SendStr(ATSTR);
        Delay_ms(500);
        strx = strstr((const char*)buf_uart1.buf, "OK");
    }
    Clear_Buffer();
    
    // GET请求
    Uart1_SendStr("AT+HTTPACTION=0\r\n");
    Delay_ms(2000);
    strx = strstr((const char*)buf_uart1.buf, "+HTTPACTION: 0,200");
    errcount = 0;
    while(strx == NULL) {
        errcount++;
        if(errcount > 10) {
            printf("***** GET FILE LIST FAILED *****\r\n");
            CSTX_HTTP_CLOSE();
            return false;
        }
        Delay_ms(1000);
        strx = strstr((const char*)buf_uart1.buf, "+HTTPACTION: 0,200");
    }
    
    strx += 19;
    uint32_t httplen = atoi(strx);
    printf("File list length: %d\r\n", httplen);
    
    if(httplen % pagebyte == 0)
        readpage = httplen / pagebyte;
    else
        readpage = httplen / pagebyte + 1;
    
    Clear_Buffer();
    bin_parser_reset();						
    
    // 读取文件列表数据
    while(readpage > 0) {
        sprintf(ATSTR, "AT+HTTPREAD=0,%d\r\n", pagebyte);
        Uart1_SendStr(ATSTR);
        
			//  动态等待，直到收到响应或超时
				errcount = 0;
				while(errcount < 100) {  // 最多等待10秒
						Delay_ms(100);
						if(strstr(buf_uart1.buf, "+HTTPREAD:") || strstr(buf_uart1.buf, "ERROR")) {
								break;
						}
					errcount++;
				}
        
        char* data_start = strstr(buf_uart1.buf, "+HTTPREAD:");
        if (!data_start) {
						printf("***** HTTPREAD *****\r\n");
            Clear_Buffer();
            continue;   //重试本页
        }
        
        data_start = strchr(data_start, '\n');
        if (!data_start) {
            Clear_Buffer();
            readpage--;
            continue;
        }
        data_start++;
        
        char* data_end = strstr(data_start, "+HTTPREAD: 0")-2;   //定位到结尾,并且要找到数据包\r\n,防止有截断数据名因为\r\n不匹配
        uint16_t data_len = data_end ? (data_end - data_start) : (buf_uart1.index - (data_start - buf_uart1.buf));
        
        if(data_len > 0) {
            bin_parser_feed_data(data_start, data_len);
        }
        
        Clear_Buffer();
        readpage--;
    }
    
    return true;
}

/*****************************************************	
下载文件并写入FLASH
参数一: *filename 文件名
参数二: flash_start_addr 写入FLASH地址
*****************************************************/
bool CSTX_HTTP_DownloadFile(const char* filename, uint32_t flash_start_addr) {
    // 设置文件URL
		uint32_t file_size = 0;

    uint32_t flash_addr = flash_start_addr;
	
		memset(ATSTR,0,sizeof(ATSTR));
		//测试服务器
    //sprintf(ATSTR, "AT+HTTPPARA=\"URL\",\"http://1oi1711bc9202.vicp.fun/weather/%s\"\r\n", filename);			
		//正式服务器
	#ifdef SOIL
		sprintf(ATSTR, "AT+HTTPPARA=\"URL\",\"http://39.91.167.37:9000/hardwaseconfig/soil/%s\"\r\n", filename);
	#endif
	#ifdef WEATHER
		sprintf(ATSTR, "AT+HTTPPARA=\"URL\",\"http://39.91.167.37:9000/hardwaseconfig/weather/%s\"\r\n", filename);
	#endif
    Uart1_SendStr(ATSTR);
    Delay_ms(1000);
    
    strx = strstr((const char*)buf_uart1.buf, "OK");
    if(strx == NULL) {
        printf("*****SET FILE URL FAILED*****\r\n");
        return false;
    }
    Clear_Buffer();
    
    // GET请求
    Uart1_SendStr("AT+HTTPACTION=0\r\n");
    Delay_ms(3000);
    
    strx = strstr((const char*)buf_uart1.buf, "+HTTPACTION: 0,200");
    errcount = 0;
    while(strx == NULL) {
        errcount++;
        if(errcount > 10) {
            printf("****DOWNLOAD FILE FAILED****\r\n");
            return false;
        }
        Delay_ms(1000);
        strx = strstr((const char*)buf_uart1.buf, "+HTTPACTION: 0,200");
    }
   // Clear_Buffer();
    
 // 解析文件大小
    strx += 19; // 跳过 "+HTTPACTION: 0,200,"
    file_size = atoi(strx);
    printf("File size: %lu bytes\r\n", file_size);
    Clear_Buffer();
    if(file_size == 0) {
        printf("****FILE SIZE IS ZERO****\r\n");
        CSTX_HTTP_CLOSE();
        return false;
    }
    Clear_Buffer();
    
    // 4. 擦除Flash区域
    uint32_t pages_to_erase = (file_size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
    printf("Erasing %lu pages...\r\n", pages_to_erase);
    for(uint32_t i = 0; i < pages_to_erase; i++) {
        Flash_ErasePage(flash_start_addr + i * FLASH_PAGE_SIZE);
    }
    
    // 5. 循环读取数据并写入Flash
    uint16_t chunk_size;
    uint32_t total_bytes = 0;
		uint16_t retry_count = 0;
    
    while(total_bytes < file_size) {
        // 计算本次读取的数据块大小
        chunk_size = (file_size - total_bytes) > pagebyte ? 
                     pagebyte : (file_size - total_bytes);
        
        // 发送HTTPREAD命令读取数据块
				memset(ATSTR,0,sizeof(ATSTR));
        sprintf(ATSTR, "AT+HTTPREAD=%lu,%d\r\n", total_bytes, chunk_size);
        Uart1_SendStr(ATSTR);
        Delay_ms(2000); // 等待模块响应
        
        // 6. 解析HTTPREAD响应
        // 典型响应格式：
        // +HTTPREAD: <len>
        // <data>
        // OK
        
        char* httpread_hdr = strstr(buf_uart1.buf, "+HTTPREAD:");
        if(!httpread_hdr) {
            printf("****HTTPREAD HEADER NOT FOUND****\r\n");
						retry_count++;
            if(retry_count > 3) return false;
            Clear_Buffer();
            continue;

        }
        
        // 解析期望的数据长度
        char* len_start = httpread_hdr + 10; // 跳过 "+HTTPREAD:"
        while(*len_start == ' ') len_start++;
        uint16_t expected_len = atoi(len_start);
        
        if(expected_len == 0|| expected_len > chunk_size) {
           printf("****INVALID LENGTH: %d****\r\n", expected_len);
            Clear_Buffer();
            break;
        }
        
        // 找到数据起始位置（在\r\n之后）
        char* data_start = strstr(httpread_hdr, "\r\n");
        if(!data_start) {
            printf("****DATA START NOT FOUND****\r\n");
						Clear_Buffer();
            break;
        }
        data_start += 2; // 跳过\r\n
        
        //  只读取expected_len字节，不依赖buf_uart1.index
        uint16_t data_len = expected_len;  // 直接等于期望长度
        
        // 检查缓冲区是否有足够数据
        int remaining_bytes = buf_uart1.index - (data_start - buf_uart1.buf);
        if(remaining_bytes < expected_len) {
            printf("****DATA INCOMPLETE: %d/%d bytes****\r\n", remaining_bytes, expected_len);
            // 数据不完整，可能需要等待更多数据或重试
            Delay_ms(500);
            continue;
        }
				
				// ===== 可选：验证帧尾"OK\r\n"（更严格）=====
        // 检查数据后面是否紧跟"OK\r\n"
        char* frame_end = data_start + expected_len;
        if(strncmp(frame_end, "OK\r\n", 4) != 0 && 
           strncmp(frame_end, "\r\nOK\r\n", 6) != 0) {
            printf("****WARNING: Frame end marker not found, data may be incomplete****\r\n");
            // 可以选择继续或重试
        }
        
        // 写入Flash
        printf("Writing block: offset=%lu, size=%d\r\n", total_bytes, data_len);
        Flash_WriteBuffer(flash_addr, (uint8_t*)data_start, data_len);
        
        flash_addr += data_len;
        total_bytes += data_len;
        retry_count = 0;  // 成功后重置重试计数
        
        printf("Progress: %lu/%lu bytes (%.1f%%)\r\n", 
               total_bytes, file_size, 
               (float)total_bytes * 100 / file_size);
        
        Clear_Buffer();
        Delay_ms(100);  // 给模块一点准备时间
    }
    
    
    //  验证下载完整性
    if(total_bytes == file_size) {
        printf("****DOWNLOAD SUCCESS: %lu bytes written to Flash****\r\n", total_bytes);
        return true;
    } else {
        printf("****DOWNLOAD FAILED: Only %lu/%lu bytes received****\r\n", total_bytes, file_size);
        NVIC_SystemReset();
    }
}

/*****************************************************	
进入服务器并下载最新相关文件
*****************************************************/
bool DownloadAndUpdateFlash(void) {
    printf("\r\n==== Starting Update Process ====\r\n");
    
    // 1. 获取文件列表
    if(!CSTX_HTTP_GetFileList()) {
        printf("****GET FILE LIST FAILED****\r\n");
        CSTX_HTTP_CLOSE();
        return false;
    }
    
    // 2. 解析并选择最新文件
    const BinFileLIST* files = bin_parser_get_result();
    printf("Found %d .bin files\r\n", files->count);
    
    for(int i = 0; i < files->count; i++) {
        printf("  - %s\r\n", files->filenames[i]);
    }
    
    const char* latest_file = find_latest_file_string(files);
    if(!latest_file) {
        printf("****NO VALID FILE FOUND****\r\n");
        CSTX_HTTP_CLOSE();
        return false;
    }
    
    printf("Selected file: %s\r\n", latest_file);
		printf("\r\n%s\r\n%s\r\n",HttpVersion,latest_file);
    if(strncmp((const char *)HttpVersion,latest_file,strlen((const char *)HttpVersion))>=0)
				return false;
    // 3. 直接下载并写入Flash（不再经过文件系统）
    if(!CSTX_HTTP_DownloadFile(latest_file, FLASH_BASE_ADDR)) {
        printf("****DOWNLOAD AND UPDATE FAILED****\r\n");
				CSTX_HTTP_CLOSE();
        NVIC_SystemReset();
    }
    CSTX_HTTP_CLOSE();
    printf("==== Update Process Complete ====\r\n");
    return true;
}


/*****************************************************
发送数据函数
参数一: *len 数据长度
参数二: *data 字符串数据
*****************************************************/
void CSTX_4G_Senddata(uint8_t *len, const char *data) 
{	
    memset(ATSTR, 0, BUFLEN);                                      //清空缓冲区
    sprintf(ATSTR, "AT+CIPSEND=0,%s\r\n", len);										 //数据加载到发送缓冲区 
    Uart1_SendStr(ATSTR);																					 //发送数据
    Delay_ms(500);
    //等待模块反馈 >
    strx = strstr((const char*)buf_uart1.buf, (const char*)">");   //模块反馈可以发送数据了
		errcount = 0;																									 //错误计数
    while(strx == NULL)
    {
        errcount++;
        strx = strstr((const char*)buf_uart1.buf, (const char*)">"); //模块反馈可以发送数据了 
				Delay_ms(500);
        if(errcount >=10)   																				 //防止死循环跳出
        {
            errcount = 0;
						Web = Web_ERROR;																				//	网络状态标志设为错误
            break;
        }
    }
		if(Web == Web_ERROR){																					//	判断如果是TCP连接误断开无法排除原因
			printf("***** SAEND  ERROR *****\r\n");
			Delay_s(30);																								//	延时三十秒直接触发看门狗重启
		}
    Uart1_SendStr((char *)data);																			//	发送真正的数据
    Delay_ms(500);

    strx = strstr((const char*)buf_uart1.buf, (const char*)"OK"); 		//	检查是否发送成功
    errcount = 0;
    while(strx == NULL)
    {
        errcount++;
        strx = strstr((const char*)buf_uart1.buf, (const char*)"OK"); //	检查是否发送成功
        Delay_ms(500);
        if(errcount >=10)   																					//	超时退出死循环 表示服务器连接失败
        {
            errcount = 0;
						Web = Web_ERROR;																			//	TCP状态标志设为TCP错误
            break;
        }
    }
    Clear_Buffer();
		if(Web == Web_ERROR){																					//	判断如果是TCP连接误断开无法排除原因
			printf("***** SAEND  ERROR *****\r\n");
			Delay_s(30);																								//	延时三十秒直接触发看门狗重启
	}
}


/*****************************************************
	函数: 4G模块PWRKEY与RESET引脚初始化    
	参数: 无
	说明: 需紧跟JTAC失能防止定电平脉冲开机
*****************************************************/
void GPRS_GPIO_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能GPIOA
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//使能GPIOC
	GPIO_InitTypeDef GPIO_InitStructure;		
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	GPIO_ResetBits(GPIOA,GPIO_Pin_15);										//拉高RESET

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);			
	GPIO_ResetBits(GPIOC,GPIO_Pin_10);										//拉高PWRKEY
	
}

/*****************************************************
	函数: 4G关机
	参数: 无
	说明: 4G模块关机时序，PWRKEY>2.5s的低电平脉冲
*****************************************************/
void GPRS_OFF(void)
{
	GPIO_SetBits(GPIOC,GPIO_Pin_10);				// 拉低PWRKEY
	Delay_ms(3000);  												// 延时3s确保稳定		
	GPIO_ResetBits(GPIOC,GPIO_Pin_10);			// 拉高PWRKEY
}

/*****************************************************
	函数: 4G开机
	参数: 无
	说明: 4G模块开机时序，PWRKEY>50ms的低电平脉冲
*****************************************************/
void GPRS_ON(void)
{
	GPIO_SetBits(GPIOC,GPIO_Pin_10);			// 拉低PWRKEY
	Delay_ms(100);  											// 延时100ms确保稳定		
	GPIO_ResetBits(GPIOC,GPIO_Pin_10);		// 拉高PWRKEY
}

