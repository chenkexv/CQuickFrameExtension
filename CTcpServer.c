/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_WIN32

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_smart_str_public.h"
#include "ext/standard/php_smart_str.h"
#include "php_CQuickFramework.h"
#include "php_CWebApp.h"
#include "php_CController.h"
#include "php_CApplication.h"
#include "php_CException.h"
#include "php_CTcpServer.h"
#include "php_CHttpServer.h"
#include <sys/epoll.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h> 
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/prctl.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/shm.h>
#include <semaphore.h>

#define MSG_USER 1
#define MSG_HEART 2

#define MAX_SENDLIST_LEN  1024

//������ȫ�ֱ���
static zval *serverSocketList = NULL;
static smart_str serverTempBuffer[10240] = {0};
static zval *serverErrorHandler = NULL;
static zval *serverSendList = NULL;
static zval *serverMainObject = NULL;
static int serverListenSocket = 0;
static sem_t *serverAccpetLock = NULL;



//�ͻ���ȫ�ֱ���
static int clientErrNums = 0;
static int clientStatus = 0;	//0 δ���� 1���ӳɹ� 2�Ͽ����ӣ� 3�������� 4�����Ͽ���������
static int clientSocketId = 0;
static unsigned int clientTimer = 0;
static char *clientHost = NULL;
static int clientPort = 0;
static double clientLastMessageTime = 0.0;
static int clientRecProcessId = 0;
static int clientMainEpollFd = 0;
static zval *clientPHPObject = NULL;
static zval *clientErrorHandler = NULL;
static int clientRecProcessWritePipeFd = 0;
static int clientRecProcessReadPipeFd = 0;
static int clientSleepFlag = 0;
static zval *clientSendList = NULL;


//zend�෽��
zend_function_entry CTcpServer_functions[] = {
	PHP_ME(CTcpServer,__construct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(CTcpServer,__destruct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(CTcpServer,getInstance,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(CTcpServer,bind,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CTcpServer,setWorker,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CTcpServer,listen,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpServer,on,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpServer,onData,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpServer,onError,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpServer,Action_worker,NULL,ZEND_ACC_PUBLIC )
	{NULL, NULL, NULL}
};

zend_function_entry CTcpClient_functions[] = {
	PHP_ME(CTcpClient,__construct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(CTcpClient,__destruct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(CTcpClient,getInstance,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(CTcpClient,connect,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CTcpClient,on,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpClient,onError,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpClient,emit,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpClient,close,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpClient,sleep,NULL,ZEND_ACC_PUBLIC )
	{NULL, NULL, NULL}
};

zend_function_entry CSocket_functions[] = {
	PHP_ME(CSocket,close,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,client,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocket,read,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocket,emit,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocket,getSocketId,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocket,getRemoteIp,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocket,getConnectTime,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocket,getSessionId,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocket,getProcessId,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocket,getLastActiveTime,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocket,getSocket,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocket,getAllConnection,NULL,ZEND_ACC_PUBLIC )
	{NULL, NULL, NULL}
};


//ģ�鱻����ʱ
CMYFRAME_REGISTER_CLASS_RUN(CTcpServer)
{
	zend_class_entry funCe;
	INIT_CLASS_ENTRY(funCe,"CTcpServer",CTcpServer_functions);
	CTcpServerCe = zend_register_internal_class_ex(&funCe,CControllerCe,NULL TSRMLS_CC);


	zend_declare_property_null(CTcpServerCe, ZEND_STRL("instance"),ZEND_ACC_PRIVATE|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_string(CTcpServerCe, ZEND_STRL("host"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CTcpServerCe, ZEND_STRL("port"),0,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CTcpServerCe, ZEND_STRL("worker"),0,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CTcpServerCe, ZEND_STRL("pidList"),ZEND_ACC_PRIVATE TSRMLS_CC);
	
	//dataCallbackObject
	zend_declare_property_null(CTcpServerCe, ZEND_STRL("eventTable"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CTcpServerCe, ZEND_STRL("eventDefault"),ZEND_ACC_PRIVATE TSRMLS_CC);

	return SUCCESS;
}

CMYFRAME_REGISTER_CLASS_RUN(CTcpClient)
{
	zend_class_entry funCe;
	INIT_CLASS_ENTRY(funCe,"CTcpClient",CTcpClient_functions);
	CTcpClientCe = zend_register_internal_class(&funCe TSRMLS_CC);

	zend_declare_property_null(CTcpClientCe, ZEND_STRL("instance"),ZEND_ACC_PRIVATE|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_string(CTcpClientCe, ZEND_STRL("host"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CTcpClientCe, ZEND_STRL("port"),0,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_double(CTcpClientCe, ZEND_STRL("connectTime"),0.00,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CTcpClientCe, ZEND_STRL("eventTable"),ZEND_ACC_PRIVATE TSRMLS_CC);

	return SUCCESS;
}

CMYFRAME_REGISTER_CLASS_RUN(CSocket)
{
	zend_class_entry funCe;
	INIT_CLASS_ENTRY(funCe,"CSocket",CSocket_functions);
	CSocketCe = zend_register_internal_class(&funCe TSRMLS_CC);


	zend_declare_property_string(CSocketCe, ZEND_STRL("message"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CSocketCe, ZEND_STRL("socketId"),0,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CSocketCe, ZEND_STRL("remoteIp"),"0.0.0.0",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_double(CSocketCe, ZEND_STRL("connectTime"),0.00,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_double(CSocketCe, ZEND_STRL("lastActiveTime"),0.00,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CSocketCe, ZEND_STRL("processId"),0,ZEND_ACC_PRIVATE TSRMLS_CC);


	//socket��������  1Ϊ������ʹ�õĶ���  2Ϊ�ͻ���ʹ�õĶ���
	zend_declare_property_long(CSocketCe, ZEND_STRL("socketType"),0,ZEND_ACC_PRIVATE TSRMLS_CC);
	return SUCCESS;
}

int startTcpListen(char *host,int port){

	int sock = socket(AF_INET,SOCK_STREAM,0);
    if( sock < 0 ){
		return -2;
    }

    //if TIME_WAIT server will restart
    int opt = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);// ��ַΪ��������
    local.sin_port = htons(port);

	//��Ϊ������
	fcntl(sock, F_SETFL, fcntl(sock, F_GETFD, 0)|O_NONBLOCK);

    if( bind(sock,(struct sockaddr *)&local,sizeof(local)) < 0 ){
		return -3;
    }

    if( listen(sock,1020) < 0 ){
		return -4;
    }
    return sock;
}


//������ �������worker����״̬
void createWorkProcess(int forkNum,int listenfd,sem_t *lock,zval *object TSRMLS_DC);

int checkTcpServerWorkerStatus(zval *object TSRMLS_DC){

	//��鵱ǰ����״̬
	int		i,h,processStatus,aliveNum = 0,
			needCreate = 0,
			need = 0;
	zval	*pidList;
	char	*key;
	ulong	uKey;

	pidList = zend_read_property(CTcpServerCe,serverMainObject,ZEND_STRL("pidList"),1 TSRMLS_CC);
	h = zend_hash_num_elements(Z_ARRVAL_P(pidList));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(pidList));
	aliveNum = 0;
	for(i = 0 ; i < h ; i++){
		zend_hash_get_current_key(Z_ARRVAL_P(pidList),&key,&uKey,0);
		//�����̴��
		processStatus = -1;
		processStatus = kill(uKey,0);
		if(processStatus == 0){
			aliveNum++;
		}else{
			zend_hash_index_del(Z_ARRVAL_P(pidList),uKey);
		}
		zend_hash_move_forward(Z_ARRVAL_P(pidList));
	}

	need = getCreateWorkerNums(object TSRMLS_CC);

	if(need > aliveNum){
		needCreate = need - aliveNum;
		php_printf("[CTcpServer] The master check child status,now alive [%d],need keep num[%d],need create[%d]\n",aliveNum,need,needCreate);
		createWorkProcess(needCreate,serverListenSocket,serverAccpetLock,serverMainObject TSRMLS_CC);
	}
	

}

void catchTcpChildSig(int sig){
	php_printf("[CTcpServer] [%d-%d] Receive a child process exit signal [%d]\n",getppid(),getpid(),sig);
	int endPid = wait(NULL);
	php_printf("[CTcpServer] The process for determining the unexpected termination is [%d]\n",endPid);

	zval	*pidList = zend_read_property(CTcpServerCe,serverMainObject,ZEND_STRL("pidList"),1 TSRMLS_CC);
	if(IS_ARRAY == Z_TYPE_P(pidList) && zend_hash_index_exists(Z_ARRVAL_P(pidList),endPid)){
		zend_hash_index_del(Z_ARRVAL_P(pidList),endPid);
	}
	checkTcpServerWorkerStatus(serverMainObject TSRMLS_CC);
}

//��������ӽ���
void createWorkProcess(int forkNum,int listenfd,sem_t *lock,zval *object TSRMLS_DC){

	int		i = 0;

	zval	*pidList = zend_read_property(CTcpServerCe,object,ZEND_STRL("pidList"),1 TSRMLS_CC);

	for(i = 0 ; i < forkNum ;i++){

		int forkPid = -1;
		forkPid=fork();
		if(forkPid==-1){
			continue;
		}else if(forkPid == 0){

			//��ʼ���ӽ���
			workProcessInit(listenfd,lock,object TSRMLS_CC);

		}else{

			signal(SIGCHLD, catchTcpChildSig);

			//�������ӽ��̵�PID����
			add_index_long(pidList,forkPid,1);

			if(i == forkNum - 1){

				while(1){
					sleep(10);
				}

			}

		}
	}

}

void catchTcpClientChildSig(int sig){
	php_printf("[CTcpServer] [%d-%d] Receive a child process exit signal [%d]\n",getppid(),getpid(),sig);
	int endPid = wait(NULL);
	php_printf("[CTcpServer] The process for determining the unexpected termination is [%d]\n",endPid);
	if(endPid == clientRecProcessId){
		if(clientStatus != 4){
			clientStatus = 2;
		}
	}
}

static int read_to_buf(const char *filename, void *buf, int len)
{
	int fd;
	int ret;
	
	if(buf == NULL || len < 0){
		printf("%s: illegal para\n", __func__);
		return -1;
	}
 
	memset(buf, 0, len);
	fd = open(filename, O_RDONLY);
	if(fd < 0){
		perror("open:");
		return -1;
	}
	ret = read(fd, buf, len);
	close(fd);
	return ret;
}
 
static char *getCmdline(int pid, char *buf, int len)
{
	char filename[32];
	char *name = NULL;
	int n = 0;
	
	if(pid < 1 || buf == NULL || len < 0){
		printf("%s: illegal para\n", __func__);
		return NULL;
	}
		
	snprintf(filename, 32, "/proc/%d/cmdline", pid);
	n = read_to_buf(filename, buf, len);
	if(n < 0)
		return NULL;
 
	if(buf[n-1]=='\n')
		buf[--n] = 0;
 
	name = buf;
	while(n) {
		if(((unsigned char)*name) < ' ')
			*name = ' ';
		name++;
		n--;
	}
	*name = 0;
	name = NULL;
 
	if(buf[0])
		return buf;
 
	return NULL;	
}

int throwClientException(char *message){

}

int throwServerException(char *message){
	if(serverErrorHandler == NULL){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,message);
		return 1;
	}

	//���������¼�
	zval	*callback;
	char	*callback_name = NULL;
	callback = serverErrorHandler;
	if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
		efree(callback_name);
		zend_throw_exception(CMicroServerExceptionCe, message, 10000 TSRMLS_CC);
		return 0;
	}

	zval	constructVal,
			contruReturn,
			*paramsList[1];

	INIT_ZVAL(constructVal);
	ZVAL_STRING(&constructVal,callback_name,0);
	MAKE_STD_ZVAL(paramsList[0]);
	ZVAL_STRING(paramsList[0],message,1);
	int callStatus = call_user_function(NULL, &callback, &constructVal, &contruReturn,1, paramsList TSRMLS_CC);

	zval_dtor(&contruReturn);
	zval_ptr_dtor(&paramsList[0]);
	efree(callback_name);
	return 2;
}

void getSocketRemoteIp(int fd,char **remoteIp){

	 struct sockaddr_in clientaddr1;
     memset(&clientaddr1, 0x00, sizeof(clientaddr1));
     socklen_t nl=sizeof(clientaddr1);
     getpeername(fd,(struct sockaddr*)&clientaddr1,&nl);
     *remoteIp = estrdup(inet_ntoa(clientaddr1.sin_addr)); 
}

int processMessage(int fd,char *thisMessage,zval *object TSRMLS_DC){

	char	*base64Decoder;
	zval	*jsonDecoder,
			*eventTable,
			*eventDefault,
			**messageEvent,
			**eventCallback,
			**stringMessage;

	base64Decode(thisMessage,&base64Decoder);
	json_decode(base64Decoder,&jsonDecoder);

	if(zend_hash_num_elements(Z_ARRVAL_P(jsonDecoder)) != 3){
		efree(base64Decoder);
		zval_ptr_dtor(&jsonDecoder);
		char *errorMessage;
		spprintf(&errorMessage,0,"some message parse error : %s",thisMessage);
		throwServerException(errorMessage);
		efree(errorMessage);
		return -1;
	}

	//��ǰ���¼�����
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),1,(void**)&messageEvent);
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),2,(void**)&stringMessage);

	//����Ƿ����ָ����event
	eventTable = zend_read_property(CTcpServerCe,object,ZEND_STRL("eventTable"), 0 TSRMLS_CC);
	eventDefault = zend_read_property(CTcpServerCe,object,ZEND_STRL("eventDefault"), 0 TSRMLS_CC);

	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(eventTable),Z_STRVAL_PP(messageEvent),strlen(Z_STRVAL_PP(messageEvent))+1,(void**)&eventCallback) && IS_OBJECT == Z_TYPE_PP(eventCallback) ){

		//��PHP��ص�
		zval	*callParams;

		MAKE_STD_ZVAL(callParams);
		object_init_ex(callParams,CSocketCe);
		zend_update_property_string(CSocketCe,callParams,ZEND_STRL("message"),Z_STRVAL_PP(stringMessage) TSRMLS_CC);
		zend_update_property_long(CSocketCe,callParams,ZEND_STRL("socketId"),fd TSRMLS_CC);
		zend_update_property_long(CSocketCe,callParams,ZEND_STRL("socketType"),1 TSRMLS_CC);
		zend_update_property_long(CSocketCe,callParams,ZEND_STRL("processId"),getpid() TSRMLS_CC);

		//���ɿͻ�����Ϣ
		zval **socketInfo;
		char socketIndex[100];
		sprintf(socketIndex,"%d_%d",getpid(),fd);
		if(SUCCESS == zend_hash_find(Z_ARRVAL_P(serverSocketList),socketIndex,strlen(socketIndex)+1,(void**)&socketInfo) && IS_ARRAY == Z_TYPE_PP(socketInfo)){
			zval **thisInfoItem;
			if(SUCCESS == zend_hash_find(Z_ARRVAL_PP(socketInfo),"remoteIp",strlen("remoteIp")+1,(void**)&thisInfoItem)){
				zend_update_property_string(CSocketCe,callParams,ZEND_STRL("remoteIp"),Z_STRVAL_PP(thisInfoItem) TSRMLS_CC);
			}
			if(SUCCESS == zend_hash_find(Z_ARRVAL_PP(socketInfo),"connectTime",strlen("connectTime")+1,(void**)&thisInfoItem)){
				zend_update_property_double(CSocketCe,callParams,ZEND_STRL("connectTime"),Z_DVAL_PP(thisInfoItem) TSRMLS_CC);
			}
		}



		char	*callback_name = NULL;
		zval	*callback;
		callback = *eventCallback;
		if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
			efree(callback_name);
			efree(base64Decoder);
			zval_ptr_dtor(&callParams);
			zval_ptr_dtor(&jsonDecoder);
			return 0;
		}

		zval	constructVal,
				contruReturn,
				*paramsList[1];

		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,callback_name,0);
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_ZVAL(paramsList[0],callParams,1,0);
		int callStatus = call_user_function(NULL, &callback, &constructVal, &contruReturn,1, paramsList TSRMLS_CC);

		zval_dtor(&contruReturn);
		zval_ptr_dtor(&paramsList[0]);
		zval_ptr_dtor(&callParams);
		efree(callback_name);
	}

	efree(base64Decoder);
	zval_ptr_dtor(&jsonDecoder);
}

//�������رտͻ�������
void serverCloseClientSocket(int socketFd,zval *object){
	
	//���socket�б�
	char socketIndex[100];
	sprintf(socketIndex,"%d_%d",getpid(),socketFd);

	if(IS_ARRAY == Z_TYPE_P(serverSocketList) && zend_hash_exists(Z_ARRVAL_P(serverSocketList),socketIndex,strlen(socketIndex)+1) ){
		//�����ر��¼�
		//WzIsImRpc2Nvbm5lY3QiLCIiXQ==  [2,"disconnect",""]
		processMessage(socketFd,"WzIsImRpc2Nvbm5lY3QiLCIiXQ==",object TSRMLS_CC);

		zend_hash_del(Z_ARRVAL_P(serverSocketList),socketIndex,strlen(socketIndex)+1);
	}

	//ɾ�����д�socketδ�ɹ����͵ļ�¼
	if(serverSendList != NULL && IS_ARRAY == Z_TYPE_P(serverSendList) && zend_hash_index_exists(Z_ARRVAL_P(serverSendList),socketFd) ){
		zend_hash_index_del(Z_ARRVAL_P(serverSendList),socketFd);
	}


	//�ر�socket
	close(socketFd);
	smart_str_0(&serverTempBuffer[socketFd]);
	smart_str_free(&serverTempBuffer[socketFd]);

}

int handlerTCPEvents(int epfd,struct epoll_event revs[],int num,int listen_sock,sem_t *lock,int isGotLock,zval *object TSRMLS_CC)
{
    struct epoll_event ev;
    int i = 0;
	int nowIsGetLock = isGotLock;
    for( ; i < num; i++ ){

		int fd = revs[i].data.fd;

		// ����accept����������
		if( fd == listen_sock && (revs[i].events & EPOLLIN) ){

			struct sockaddr_in client;
			socklen_t len = sizeof(client);
			extern int errno;
			int new_sock = accept(fd,(struct sockaddr *)&client,&len);

			if( new_sock < 0 ){
				if(errno == 11){
				}else{
					setLogs("[%d] accept fail,errorno:%d \n",getpid(),errno);
				}
				continue;
			}

			//socket��Ϊ������
			fcntl(new_sock, F_SETFL, fcntl(new_sock, F_GETFL, NULL) | O_NONBLOCK);

			//��ȡ�Զ���Ϣ
			zval	*remoteInfo,
					*connectTime;
			char	*remoteIp;
			getSocketRemoteIp(new_sock,&remoteIp);
			MAKE_STD_ZVAL(remoteInfo);
			array_init(remoteInfo);
			microtime(&connectTime);
			add_assoc_string(remoteInfo,"remoteIp",remoteIp,0);
			add_assoc_double(remoteInfo,"connectTime",Z_DVAL_P(connectTime));
			zval_ptr_dtor(&connectTime);
			char socketIndex[100];
			sprintf(socketIndex,"%d_%d",getpid(),new_sock);
			add_assoc_zval(serverSocketList,socketIndex,remoteInfo);

			//���ϵͳĬ���¼�����
			//WzIsImNvbm5lY3QiLCIiXQ==  [2,"connect",""]
			processMessage(new_sock,"WzIsImNvbm5lY3QiLCIiXQ==",object TSRMLS_CC);

			//�¼�����Ϣ
			ev.events = EPOLLIN;
			ev.data.fd = new_sock;
			epoll_ctl(epfd,EPOLL_CTL_ADD,new_sock,&ev);


			continue;
		}

		// �������ͨ�ļ��������������read�ṩ��ȡ���ݵķ���
		if(revs[i].events & EPOLLIN)	{

			char		buf[2],
						*thisMessage;
			int			readLen = 0,k;

			smart_str	tempBuffer[10240] = {0};

			//�ͷ��ź��� ���������̾���accept
			if(isGotLock){
				sem_post(lock);
				nowIsGetLock = 0;
			}

			while(1){
				errno = 0;
				readLen = read(fd,buf,sizeof(buf)-1);

				if(readLen <= 0){
					if(readLen == 0){
						serverCloseClientSocket(fd,object);
					}
					break;
				}

				if(buf[0] != '\n'){
					smart_str_appendc(&tempBuffer[fd],buf[0]);
				}else{
					smart_str_0(&tempBuffer[fd]);
					thisMessage = estrdup(tempBuffer[fd].c);
					smart_str_free(&tempBuffer[fd]);

					//������Ϣ
					processMessage(fd,thisMessage,object TSRMLS_CC);
					efree(thisMessage);
					break;
				}	
			}
		}

    }
	return nowIsGetLock;
}

//������д��Ϣ
void doServerSendMessage(zval *object TSRMLS_DC){

	int		i,h,j,k;
	zval	**thisMessage,
			**toSocket,
			**messageContent,
			**socketMessage;
	char	*key;
	ulong	uKey;

	if(serverSendList == NULL || IS_ARRAY != Z_TYPE_P(serverSendList)){
		return;
	}

	h = zend_hash_num_elements(Z_ARRVAL_P(serverSendList));

	if(h == 0){
		return;
	}
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(serverSendList));
	for(i = 0 ; i < h;i++){
		zend_hash_get_current_data(Z_ARRVAL_P(serverSendList),(void**)&socketMessage);

		if(IS_ARRAY == Z_TYPE_PP(socketMessage)){
			k = zend_hash_num_elements(Z_ARRVAL_PP(socketMessage));
			for(j = 0 ; j < k ;j++){

				zend_hash_get_current_key(Z_ARRVAL_PP(socketMessage),&key,&uKey,0);
				if(SUCCESS == zend_hash_get_current_data(Z_ARRVAL_PP(socketMessage),(void**)&thisMessage) && IS_ARRAY == Z_TYPE_PP(thisMessage)){
				}else{
					zend_hash_index_del(Z_ARRVAL_PP(socketMessage),uKey);
					zend_hash_move_forward(Z_ARRVAL_PP(socketMessage));
					continue;
				}


				if(SUCCESS == zend_hash_index_find(Z_ARRVAL_PP(thisMessage),0,(void**)&toSocket) && IS_LONG == Z_TYPE_PP(toSocket) &&
				   SUCCESS == zend_hash_index_find(Z_ARRVAL_PP(thisMessage),1,(void**)&messageContent) && IS_STRING == Z_TYPE_PP(messageContent)){
				}else{
					zend_hash_index_del(Z_ARRVAL_PP(socketMessage),uKey);
					zend_hash_move_forward(Z_ARRVAL_PP(socketMessage));
					continue;
				}

				errno = 0;
				int status = write(Z_LVAL_PP(toSocket),Z_STRVAL_PP(messageContent),Z_STRLEN_PP(messageContent));

				//д��ʧ��
				if(status <= 0){	
					if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
						break;
					}else{
						//�ͻ����ѹر�
						serverCloseClientSocket(Z_LVAL_PP(toSocket),object);
						break;
					}
				}
				
				//�Ƴ���Ϣ
				zend_hash_index_del(Z_ARRVAL_PP(socketMessage),uKey);
				zend_hash_move_forward(Z_ARRVAL_PP(socketMessage));
			}
		}

		zend_hash_move_forward(Z_ARRVAL_P(serverSendList));
	}

}

void doClientSendMessage(){

	int		i,h;
	zval	**thisMessage,
			**toSocket,
			**messageContent;
	char	*key;
	ulong	uKey;


	if(clientSendList == NULL || IS_ARRAY != Z_TYPE_P(clientSendList)){
		return;
	}

	h = zend_hash_num_elements(Z_ARRVAL_P(clientSendList));
	if(h == 0){
		return;
	}

	zend_hash_internal_pointer_reset(Z_ARRVAL_P(clientSendList));
	for(i = 0 ; i < h;i++){

		zend_hash_get_current_data(Z_ARRVAL_P(clientSendList),(void**)&thisMessage);
		zend_hash_get_current_key(Z_ARRVAL_P(clientSendList),&key,&uKey,0);

		zend_hash_index_find(Z_ARRVAL_PP(thisMessage),0,(void**)&toSocket);
		zend_hash_index_find(Z_ARRVAL_PP(thisMessage),1,(void**)&messageContent);


		errno = 0;
		int status = write(clientSocketId,Z_STRVAL_PP(messageContent),Z_STRLEN_PP(messageContent));

		//д��ʧ�� �ó�����
		if(status <= 0){
			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
				break;
			}else{

				//���������쳣
				ClientCloseSocket(0,0);
				break;
			}
		}

		//��д��һ���ֵ���Ϣ
		if(status < Z_STRLEN_PP(messageContent)){
			char *notSendMessage;
			substr(Z_STRVAL_PP(messageContent),status,Z_STRLEN_PP(messageContent)-status,&notSendMessage);
			add_index_string(*thisMessage,1,notSendMessage,0);
			break;
		}

		
		//�Ƴ���Ϣ
		zend_hash_move_forward(Z_ARRVAL_P(clientSendList));
		zend_hash_index_del(Z_ARRVAL_P(clientSendList),uKey);
	}

}


//��ʼ���ӽ���
//���ý��̱�ʶ
//ÿ���ӽ��̰�epoll
int workProcessInit(int listenfd,sem_t *lock,zval *object TSRMLS_DC){

	ini_set("memory_limit","196M");

	//���epoll�¼�
	int epfd = epoll_create(1024);
	if(epfd <= 0){
		//����ʧ��  �ӽ��������˳�
		return -1;
	}

	//�󶨶�����
	struct epoll_event  ev;
    ev.events = EPOLLIN; 
    ev.data.fd = listenfd;

	if(serverSendList == NULL){
		MAKE_STD_ZVAL(serverSendList);
		array_init(serverSendList);
	}

	//�����ź� ��ֹ�����˳�
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);

	
	struct epoll_event revs[128];
    int n = sizeof(revs)/sizeof(revs[0]);
    int timeout = 3000;
    int num = 0;
	int isGotLock = 0;

	//��ʼ��1��hash���¼����socket
	if(serverSocketList == NULL){
		MAKE_STD_ZVAL(serverSocketList);
		array_init(serverSocketList);
	}

	//��������
	while(1) {

		//����ź���
		errno = 0;
		int tryNum = sem_trywait(lock);
		if (tryNum == 0) {
			if (isGotLock == 0) {
				isGotLock = 1;
				//ע���¼�
				epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);

				php_printf("register\n");
			}
		}else{
			if (isGotLock) {
				//ɾ���¼�
				epoll_ctl(epfd, EPOLL_CTL_DEL, listenfd, NULL);
				isGotLock = 0;

				php_printf("unregister\n");
			}
		}


       //��ʼepoll�¼��ȴ�
       num = epoll_wait(epfd,revs,n,500);

	   php_printf("epoll:%d\n",num);

	   if(num > 0){
			isGotLock = handlerTCPEvents(epfd,revs,num,listenfd,lock,isGotLock,object TSRMLS_CC);
	   }

		doServerSendMessage(object TSRMLS_CC);

	   if (isGotLock) {
			sem_post(lock);
			isGotLock = 0;
		}
    }

    close(epfd);
    return 0;

}


PHP_METHOD(CTcpServer,__construct){
	zval	*eventTable;

	ini_set("memory_limit","196M");

	eventTable = zend_read_property(CTcpServerCe,getThis(),ZEND_STRL("eventTable"), 0 TSRMLS_CC);
	if(IS_ARRAY != Z_TYPE_P(eventTable)){
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		zend_update_property(CTcpServerCe,getThis(),ZEND_STRL("eventTable"),saveArray TSRMLS_CC);
		zval_ptr_dtor(&saveArray);
		eventTable = zend_read_property(CTcpServerCe,getThis(),ZEND_STRL("eventTable"),1 TSRMLS_CC);
	}

}

PHP_METHOD(CTcpServer,__destruct){

	if(serverErrorHandler != NULL){
		zval_ptr_dtor(&serverErrorHandler);
	}
}

PHP_METHOD(CTcpServer,getInstance){

	char	*key,
			*saveKey;
	int		keyLen = 0;

	zval	*selfInstace,
			**instaceSaveZval;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|s",&key,&keyLen) == FAILURE){
		RETURN_FALSE;
	}

	if(keyLen == 0){
		key = "main";
	}


	selfInstace = zend_read_static_property(CTcpServerCe,ZEND_STRL("instance"),1 TSRMLS_CC);

	//���ΪNULL�����ΪZvalHashtable
	if(IS_ARRAY != Z_TYPE_P(selfInstace)){
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		zend_update_static_property(CTcpServerCe,ZEND_STRL("instance"),saveArray TSRMLS_CC);
		zval_ptr_dtor(&saveArray);
		selfInstace = zend_read_static_property(CTcpServerCe,ZEND_STRL("instance"),1 TSRMLS_CC);
	}

	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(selfInstace),key,strlen(key)+1,(void**)&instaceSaveZval) ){
		RETVAL_ZVAL(*instaceSaveZval,1,0);
	}else{

		zval	*object;

		MAKE_STD_ZVAL(object);
		object_init_ex(object,CTcpServerCe);

		//ִ���乹���� ���������
		if (CTcpServerCe->constructor) {
			zval	constructVal,
					constructReturn;
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal, CTcpServerCe->constructor->common.function_name, 0);
			call_user_function(NULL, &object, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
			zval_dtor(&constructReturn);
		}

		//������������ֵ����instance��̬����
		add_assoc_zval(selfInstace,key,object);
		zend_update_static_property(CTcpServerCe,ZEND_STRL("instance"),selfInstace TSRMLS_CC);

		RETURN_ZVAL(object,1,0);
	}
}

int getCreateWorkerNums(zval *object TSRMLS_DC){
	
	zval	*workerNum;
	workerNum = zend_read_property(CTcpServerCe,object,ZEND_STRL("worker"), 0 TSRMLS_CC);

	if(IS_LONG == Z_TYPE_P(workerNum) && Z_LVAL_P(workerNum) > 0){
		return Z_LVAL_P(workerNum);
	}

	//����CPU������
	int cpuNum =  get_nprocs();
	return cpuNum*2;
}

PHP_METHOD(CTcpServer,listen){

	zval	*host,
			*port,
			*object,
			**argv,
			**SERVER,
			*pidList;

	int		errorCode = 0,
			isDaemon = 0,
			isMaster = 1;

	char	appPath[2024],
			codePath[2024];

	RETVAL_ZVAL(getThis(),1,0);

	host = zend_read_property(CTcpServerCe,getThis(),ZEND_STRL("host"), 0 TSRMLS_CC);
	port = zend_read_property(CTcpServerCe,getThis(),ZEND_STRL("port"), 0 TSRMLS_CC);

	//init framework
	getcwd(appPath,sizeof(appPath));
	sprintf(codePath,"%s%s",appPath,"/application");
	php_define("APP_PATH",appPath TSRMLS_CC);
	php_define("CODE_PATH",codePath TSRMLS_CC);
	CWebApp_createApp(&object TSRMLS_CC);
	zval_ptr_dtor(&object);


	(void)zend_hash_find(&EG(symbol_table),ZEND_STRS("_SERVER"), (void **)&SERVER);
	if(zend_hash_find(Z_ARRVAL_PP(SERVER),"argv",strlen("argv")+1,(void**)&argv) == SUCCESS && IS_ARRAY == Z_TYPE_PP(argv)){
		int	i,h;
		zval **thisVal;
		h = zend_hash_num_elements(Z_ARRVAL_PP(argv));
		zend_hash_internal_pointer_reset(Z_ARRVAL_PP(argv));
		for(i = 0 ; i < h;i++){
			zend_hash_get_current_data(Z_ARRVAL_PP(argv),(void**)&thisVal);
			if(strstr(Z_STRVAL_PP(thisVal),"--daemon") != NULL){
				isDaemon = 1;
			}
			if(strstr(Z_STRVAL_PP(thisVal),"--worker") != NULL){
				isMaster = 0;
			}
			zend_hash_move_forward(Z_ARRVAL_PP(argv));
		}
	}

	//daemon
	if(isDaemon){
		php_printf("run as a daemon process..\n");
		int s = daemon(1, 0);
	}

	//��������̳�
	pidList = zend_read_property(CTcpServerCe,getThis(),ZEND_STRL("pidList"),1 TSRMLS_CC);
	if(IS_ARRAY != Z_TYPE_P(pidList)){
		zval *saveData;
		MAKE_STD_ZVAL(saveData);
		array_init(saveData);
		zend_update_property(CTcpServerCe,getThis(),ZEND_STRL("pidList"),saveData TSRMLS_CC);
		zval_ptr_dtor(&saveData);
		pidList = zend_read_property(CTcpServerCe,getThis(),ZEND_STRL("pidList"),1 TSRMLS_CC);
	}
	
	//����socket�׽��� ����fork�ӽ��� �Թ�����׽���
	int listenSocket = startTcpListen(Z_STRVAL_P(host),Z_LVAL_P(port));
	if(listenSocket < 0){
		//�����쳣
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[FatalException] TCP Server can not bind %s:%d ",Z_STRVAL_P(host),Z_LVAL_P(port));
		return;
	}

	//����ź��� ���ڿ��ƽ��̾�Ⱥ
	sem_unlink("CQuickFrameEPoll");
    sem_t *lock = sem_open("CQuickFrameEPoll", O_CREAT, 0666, 1);
    if (lock == SEM_FAILED) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[FatalException] call sem_open fail ");
		close(listenSocket);
		return;
    }

	//ʹ��systemV�ź���
	int semid = semget((key_t)getpid(),1,IPC_CREAT|IPC_EXCL|0664);
	if(semid < 0){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[FatalException] can not create systemV semget ");
		close(listenSocket);
		return;
	}
	//��ʼ��
	int initStatus = semctl(semid , 0 , SETVAL , val);


	//����ȫ�ֱ��� ����״̬���
	serverAccpetLock = lock;
	serverListenSocket = listenSocket;
	serverMainObject = getThis();

	//fork���ӽ���
	int workerNum = getCreateWorkerNums(getThis() TSRMLS_CC);
	createWorkProcess(workerNum,listenSocket,lock,getThis() TSRMLS_CC);
}

PHP_METHOD(CTcpServer,setWorker){
	long	num = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"l",&num) == FAILURE){
		RETURN_FALSE;
	}
	zend_update_property_long(CTcpServerCe,getThis(),ZEND_STRL("worker"),num TSRMLS_CC);
	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CTcpServer,bind){
	char	*host;
	int		hostLen = 0;
	long	port = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sl",&host,&hostLen,&port) == FAILURE){
		RETURN_FALSE;
	}

	if(port == 0){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[CTCPServerException] call [CTCPServer->bind] the port must available port");
		return;
	}

	zend_update_property_string(CTcpServerCe,getThis(),ZEND_STRL("host"),host TSRMLS_CC);
	zend_update_property_long(CTcpServerCe,getThis(),ZEND_STRL("port"),port TSRMLS_CC);

	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CTcpServer,on){

	zval	*callback,
			*saveCallback;
	char	*callback_name,
			*eventName;
	int		eventNameLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sz",&eventName,&eventNameLen,&callback) == FAILURE){
		RETURN_FALSE;
	}

	if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
        efree(callback_name);
		throwServerException("[CTcpServerException] call [CTcpServer->on] the params is not a callback function");
        RETVAL_ZVAL(getThis(),1,0);
        return;
    }

	zval	*eventTable;


	//save to callbackObject
	eventTable = zend_read_property(CTcpServerCe,getThis(),ZEND_STRL("eventTable"), 0 TSRMLS_CC);
	MAKE_STD_ZVAL(saveCallback);
	ZVAL_ZVAL(saveCallback,callback,1,0);
	add_assoc_zval(eventTable,eventName,saveCallback);
	zend_update_property(CTcpServerCe,getThis(),ZEND_STRL("eventTable"),eventTable TSRMLS_CC);

    efree(callback_name);
	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CTcpServer,onData){

	zval	*callback;
	char	*callback_name;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&callback) == FAILURE){
		RETURN_FALSE;
	}

	if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
        efree(callback_name);
		throwServerException("[CTcpServerException] call [CTcpServer->onData] the params is not a callback function");
        RETVAL_ZVAL(getThis(),1,0);
        return;
    }


	//save to callbackObject
	zend_update_property(CTcpServerCe,getThis(),ZEND_STRL("eventDefault"),callback TSRMLS_CC);

    efree(callback_name);
	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CTcpServer,Action_worker){

}


PHP_METHOD(CTcpClient,__construct){

	zval *eventTable;

	ini_set("memory_limit","196M");

	eventTable = zend_read_property(CTcpClientCe,getThis(),ZEND_STRL("eventTable"), 0 TSRMLS_CC);
	if(IS_ARRAY != Z_TYPE_P(eventTable)){
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		zend_update_property(CTcpClientCe,getThis(),ZEND_STRL("eventTable"),saveArray TSRMLS_CC);
		zval_ptr_dtor(&saveArray);
		eventTable = zend_read_property(CTcpClientCe,getThis(),ZEND_STRL("eventTable"),1 TSRMLS_CC);
	}
}


PHP_METHOD(CTcpClient,__destruct){

	//�������� �ر�socket
	if(clientSocketId){

		//�ر�socket
		close(clientSocketId);
	}


	//ɾ��
	if(clientHost != NULL){
		efree(clientHost);
		clientHost = NULL;
	}

	if(clientErrorHandler != NULL){
		zval_ptr_dtor(&clientErrorHandler);
	}
}

PHP_METHOD(CTcpClient,getInstance){
	char	*key,
			*saveKey;
	int		keyLen = 0;

	zval	*selfInstace,
			**instaceSaveZval;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|s",&key,&keyLen) == FAILURE){
		RETURN_FALSE;
	}

	if(keyLen == 0){
		key = "main";
	}


	selfInstace = zend_read_static_property(CTcpClientCe,ZEND_STRL("instance"),1 TSRMLS_CC);

	//���ΪNULL�����ΪZvalHashtable
	if(IS_ARRAY != Z_TYPE_P(selfInstace)){
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		zend_update_static_property(CTcpClientCe,ZEND_STRL("instance"),saveArray TSRMLS_CC);
		zval_ptr_dtor(&saveArray);
		selfInstace = zend_read_static_property(CTcpClientCe,ZEND_STRL("instance"),1 TSRMLS_CC);
	}

	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(selfInstace),key,strlen(key)+1,(void**)&instaceSaveZval) ){
		RETVAL_ZVAL(*instaceSaveZval,1,0);
	}else{

		zval	*object;

		MAKE_STD_ZVAL(object);
		object_init_ex(object,CTcpClientCe);

		//ִ���乹���� ���������
		if (CTcpClientCe->constructor) {
			zval	constructVal,
					constructReturn;
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal, CTcpClientCe->constructor->common.function_name, 0);
			call_user_function(NULL, &object, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
			zval_dtor(&constructReturn);
		}

		//������������ֵ����instance��̬����
		add_assoc_zval(selfInstace,key,object);
		zend_update_static_property(CTcpClientCe,ZEND_STRL("instance"),selfInstace TSRMLS_CC);

		RETURN_ZVAL(object,1,0);
	}
}

//����Զ��socket
int connectServerPort(char *host,int port TSRMLS_DC){


	int sockfd, num;    
    char buf[1024];  
    struct sockaddr_in server;


	if((sockfd=socket(AF_INET,SOCK_STREAM, 0))==-1){
		return -2;
	}

	bzero(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(host);

	//��������
	if(connect(sockfd, (struct sockaddr *)&server, sizeof(server))==-1){
		return -3;
	}

	return sockfd;
}

int ClientCloseSocket(int epfd,int listen_sock){

	//�Ƴ�epoll�¼�
	if(epfd > 0){
		epoll_ctl(epfd, EPOLL_CTL_DEL, listen_sock, NULL);
	}

	//�رյ�ǰsocket
	if(listen_sock > 0){
		close(listen_sock);
	}

	//�˳� ��ǰ�ӽ���
	zend_bailout();
}

int checkServerStatus(int epfd,int listen_sock){
	
	//���Է���ȷ�ϰ�
	php_printf("server happend exception,waite to reconnect\n");

	//�ر�socket ��Ϊ�Ͽ� �ȴ�����
	ClientCloseSocket(epfd,listen_sock);


}

int processMasterSendMessage(int fd,char *thisMessage TSRMLS_DC){
	char *saveMessage;
	zval *sendMessage;
	MAKE_STD_ZVAL(sendMessage);
	array_init(sendMessage);
	add_index_long(sendMessage,0,fd);
	spprintf(&saveMessage,0,"%s%c",thisMessage,'\n');
	add_index_string(sendMessage,1,saveMessage,0);
	add_next_index_zval(clientSendList,sendMessage);
	return 1;
}

int processClientMessage(int fd,char *thisMessage TSRMLS_DC){

	char	*base64Decoder;
	zval	*jsonDecoder,
			*eventTable,
			*eventDefault,
			**messageEvent,
			**eventCallback,
			**stringMessage;

	base64Decode(thisMessage,&base64Decoder);
	json_decode(base64Decoder,&jsonDecoder);

	if(zend_hash_num_elements(Z_ARRVAL_P(jsonDecoder)) != 3){
		efree(base64Decoder);
		zval_ptr_dtor(&jsonDecoder);
		char *errorMessage;
		spprintf(&errorMessage,0,"some message parse error : %s",thisMessage);
		throwClientException(errorMessage);
		efree(errorMessage);
		return -1;
	}

	
	//��ǰ���¼�����
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),1,(void**)&messageEvent);
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),2,(void**)&stringMessage);

	//����Ƿ����ָ����event
	eventTable = zend_read_property(CTcpClientCe,clientPHPObject,ZEND_STRL("eventTable"), 0 TSRMLS_CC);

	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(eventTable),Z_STRVAL_PP(messageEvent),strlen(Z_STRVAL_PP(messageEvent))+1,(void**)&eventCallback) && IS_OBJECT == Z_TYPE_PP(eventCallback) ){

		//��PHP��ص�
		zval	*callParams,
				*connectTimeZval;

		MAKE_STD_ZVAL(callParams);
		object_init_ex(callParams,CSocketCe);
		zend_update_property_string(CSocketCe,callParams,ZEND_STRL("message"),Z_STRVAL_PP(stringMessage) TSRMLS_CC);
		zend_update_property_long(CSocketCe,callParams,ZEND_STRL("socketId"),fd TSRMLS_CC);
		zend_update_property_long(CSocketCe,callParams,ZEND_STRL("socketType"),2 TSRMLS_CC);
		zend_update_property_long(CSocketCe,callParams,ZEND_STRL("processId"),getpid() TSRMLS_CC);

		//���ɿͻ�����Ϣ
		zend_update_property_string(CSocketCe,callParams,ZEND_STRL("remoteIp"),clientHost TSRMLS_CC);

		connectTimeZval = zend_read_property(CTcpClientCe,clientPHPObject,ZEND_STRL("connectTime"), 0 TSRMLS_CC);
		zend_update_property_double(CSocketCe,callParams,ZEND_STRL("connectTime"),Z_DVAL_P(connectTimeZval) TSRMLS_CC);


		char	*callback_name = NULL;
		zval	*callback;
		callback = *eventCallback;
		if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
			efree(callback_name);
			efree(base64Decoder);
			zval_ptr_dtor(&callParams);
			zval_ptr_dtor(&jsonDecoder);
			return 0;
		}

		zval	constructVal,
				contruReturn,
				*paramsList[1];

		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,callback_name,0);
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_ZVAL(paramsList[0],callParams,1,0);
		int callStatus = call_user_function(NULL, &callback, &constructVal, &contruReturn,1, paramsList TSRMLS_CC);

		zval_dtor(&contruReturn);
		zval_ptr_dtor(&paramsList[0]);
		zval_ptr_dtor(&callParams);
		efree(callback_name);
	}


	efree(base64Decoder);
	zval_ptr_dtor(&jsonDecoder);
}

int handerTcpClientMessage(int epfd,struct epoll_event revs[],int num,int listen_sock TSRMLS_CC){

	struct epoll_event ev;
    int i = 0;
    for( ; i < num; i++ ){

		int fd = revs[i].data.fd;

		if(fd == listen_sock && revs[i].events & EPOLLIN)	{

			char		buf[2],
						*thisMessage;
			int			readLen = 0,
						k,
						findMessage = 0;
			
			while(1){
				errno = 0;
				readLen = read(fd,buf,sizeof(buf)-1);

				if(readLen <= 0){
					if(readLen == 0){
						ClientCloseSocket(fd,epfd);
					}
					break;
				}
				for(k = 0 ; k < readLen;k++){

					//���μ���ַ�
					if(buf[k] != '\n'){
						smart_str_appendc(&serverTempBuffer[fd],buf[k]);
					}else if(buf[k] == '\n' || readLen < sizeof(buf)-1){
						smart_str_0(&serverTempBuffer[fd]);
						thisMessage = estrdup(serverTempBuffer[fd].c);
						
						//������Ϣ
						processClientMessage(fd,thisMessage TSRMLS_CC);

						findMessage = 1;
						efree(thisMessage);
						smart_str_free(&serverTempBuffer[fd]);
						break;
					}
				}
				if(findMessage){
					findMessage = 0;
					break;
				}
			}
		}


		if(fd == clientRecProcessReadPipeFd && revs[i].events & EPOLLIN)	{

			char		buf[2],
						*thisMessage;
			int			readLen = 0,
						k,
						findMessage = 0;
			smart_str	masterMessage = {0};
			
			//�����������Ͷ��г���
			int sendLen = zend_hash_num_elements(Z_ARRVAL_P(clientSendList));
			if(sendLen >= MAX_SENDLIST_LEN){
				//����ȡmaster����Ϣ  ��master����
				return 1;
			}

			while(1){
				readLen = read(fd,buf,sizeof(buf)-1);

				if(readLen <= 0){
					smart_str_0(&masterMessage);
					smart_str_free(&masterMessage);
					break;
				}
				for(k = 0 ; k < readLen;k++){

					//���μ���ַ�
					if(buf[k] != '\n'){
						smart_str_appendc(&masterMessage,buf[k]);
					}else if(buf[k] == '\n' || readLen < sizeof(buf)-1){
						smart_str_0(&masterMessage);
						thisMessage = estrdup(masterMessage.c);
						
						//������Ϣ 
						processMasterSendMessage(clientSocketId,thisMessage TSRMLS_CC);

						findMessage = 1;
						efree(thisMessage);
						smart_str_free(&masterMessage);
						break;
					}
				}
				if(findMessage){
					findMessage = 0;
					break;
				}
			}
		}
    }
}

void tcpClienthandleMaster_signal(int signo){
	//�յ��������Ƴ����ź�
	if (signo == SIGHUP){  
		zend_bailout();
	}
}

int initRecProcess(int socket,int epfd,int readFd){
	//��ʼ���ӽ���
	signal(SIGHUP, tcpClienthandleMaster_signal);	//�յ��������˳��ź� �����˳�
	prctl(PR_SET_PDEATHSIG, SIGHUP);		//Ҫ�󸸽����˳�ʱ�����ź�
	prctl(PR_SET_NAME, "CTcpClientRevProcess"); 

	//�ܵ� ���¼���Ϊ������
	clientRecProcessReadPipeFd = readFd;
	fcntl(clientRecProcessReadPipeFd, F_SETFL, fcntl(clientRecProcessReadPipeFd, F_GETFL, 0) | O_NONBLOCK);


	//��ʼ�����Ͷ���
	if(clientSendList == NULL){
		MAKE_STD_ZVAL(clientSendList);
		array_init(clientSendList);
	}

	//�����ź� ��ֹ�����˳�
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);


	//socket������
	fcntl(socket, F_SETFL, fcntl(socket, F_GETFD, 0)|O_NONBLOCK);

	//����socket�¼�
	struct epoll_event  ev;
	ev.events = EPOLLIN; 
	ev.data.fd = socket;
	struct epoll_event revs[128];
	int n = sizeof(revs)/sizeof(revs[0]);
	int timeout = 3000;
	int num = 0;
	epoll_ctl(epfd,EPOLL_CTL_ADD,socket,&ev);

	//���ܵ�����epoll
	struct epoll_event  ev2;
	ev2.events = EPOLLIN; 
	ev2.data.fd = clientRecProcessReadPipeFd;
	epoll_ctl(epfd,EPOLL_CTL_ADD,clientRecProcessReadPipeFd,&ev2);


	//��������
	while(1) {

	   //��ʼepoll�¼��ȴ�
	   num = epoll_wait(epfd,revs,n,1000);

	   if(num > 0){
			
		   handerTcpClientMessage(epfd,revs,num,socket TSRMLS_CC);

	   }

	   doClientSendMessage();
	}
}


//������Ϣ
//type 1Ϊ�Զ�����Ϣ
int Client_sendMessage(int socket,char *message,int tryNums TSRMLS_DC){

	if(tryNums >= 10){
		return -1;
	}

	errno = 0;
	tryNums++;
	int writeTime = write(socket,message,strlen(message));

	//��¼���һ������socket io������ʱ��
	if(writeTime > 0){
		clientLastMessageTime = getMicrotime();
	}

	//socket����ʧЧ
	if(writeTime < 0){

		if(errno == 1 || errno == 4){
		}else if(errno == 11){
			usleep(500);
			return Client_sendMessage(socket,message,tryNums TSRMLS_CC);
		}else{
			kill(clientRecProcessId,SIGKILL);
		}
	}

	return writeTime;
}

void timerChecker(void(*prompt_info)())
{
	struct sigaction tact;
	memset(&tact, 0, sizeof(tact));
	tact.sa_handler = prompt_info;
	tact.sa_flags = 0;
	sigemptyset(&tact.sa_mask);
	sigaction(SIGALRM, &tact, NULL);
}


void initTimer(){
	struct itimerval value;
	//�趨ִ�������ʱ����Ϊ1��0΢��
	value.it_value.tv_sec = 1;
	value.it_value.tv_usec = 0;
	value.it_interval = value.it_value;
	setitimer(ITIMER_REAL, &value, NULL);
}


//����socket
int resetSocketConnect(){

	php_printf("[%d] is try to reconnect server,old socket[%d] ...\n",getpid(),clientSocketId);

	//�ر�socket
	close(clientSocketId);
	
	//��������
	int socket = connectServerPort(clientHost,clientPort TSRMLS_CC);
	if(socket < 0){
		clientStatus = 2;
		php_printf("[%d] is try to reconnect server Fail ...\n",getpid());
		return -1;
	}

	clientStatus = 1;
	clientSocketId = socket;

	//�����ܵ�
	int masterWriteFd[2];
	if(pipe(masterWriteFd) < 0){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[CTCPServerException] can not create pipe..");
		return;
	}

	//fork �ӽ��� ����ѭ������Ϣ
	int i = 0;
	for(i = 0 ; i < 1 ;i++){

		int forkPid = -1;
		forkPid=fork();
		if(forkPid==-1){

		}else if(forkPid == 0){

			close(masterWriteFd[1]);
			initRecProcess(socket,clientMainEpollFd,masterWriteFd[0]);

		}else{

			clientRecProcessWritePipeFd = masterWriteFd[1];
			close(masterWriteFd[0]);

			clientRecProcessId = forkPid;

		}
	}

	php_printf("[%d] is try to reconnect server Success,new socket[%d] ...\n",getpid(),socket);
	clientStatus = 1;
	return 1;
}

void checkSocketStatus(){


	if(clientStatus == 2){

		//��socket״̬Ϊ �Ͽ�ʱ����Ϊ��������
		clientStatus == 3;

		//����socket
		int restetStatus = resetSocketConnect();

	}
}

//�ͻ�������
void clientHeartbeat(){

	//������ʱ�����60�� ������һ��


}

void timerCallback(){

	//ÿ5�� ���socket״̬
	if(clientTimer % 5 == 0){

		//���socket״̬
		checkSocketStatus();

	}
	
	//30�����
	if(clientTimer % 30 == 0){

		//�ͻ�������
		clientHeartbeat();
	}

	if(clientSleepFlag > 0){
		clientSleepFlag--;
	}


	//��ʱ���ۼ�
	clientTimer++;
	if(clientTimer >= 1000000000){
		clientTimer = 0;
	}

}

void clientProcessExit(void){
	
}

PHP_METHOD(CTcpClient,connect){
	
	char	*host = NULL;
	int		hostLen = 0;
	long	port = 0;



	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sl",&host,&hostLen,&port) == FAILURE){
		RETURN_FALSE;
	}

	RETVAL_ZVAL(getThis(),1,0);

	zend_update_property_string(CTcpClientCe,getThis(),ZEND_STRL("host"),host TSRMLS_CC);
	zend_update_property_long(CTcpClientCe,getThis(),ZEND_STRL("port"),port TSRMLS_CC);

	//��¼ȫ�ֱ���
	clientHost = estrdup(host);
	clientPort = port;

	//���ӷ�����
	int socket = connectServerPort(host,port TSRMLS_CC);
	if(socket < 0){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[FatalException] TCP Client can connect to %s:%d ,errorno : %d",host,port,socket);
		return;
	}

	clientStatus = 1;
	clientSocketId = socket;

	//��¼����ʱ��
	zval *connectTimeZval;
	microtime(&connectTimeZval);
	zend_update_property_double(CTcpClientCe,getThis(),ZEND_STRL("connectTime"),Z_DVAL_P(connectTimeZval) TSRMLS_CC);
	zval_ptr_dtor(&connectTimeZval);

	if(clientPHPObject == NULL){
		clientPHPObject = getThis();
	}

	//��ʼѭ���ȴ�
	clientMainEpollFd = epoll_create(1024);
	if(clientMainEpollFd <= 0){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[FatalException] TCP Client can connect create epoll event");
		return;
	}

	//�����ܵ�
	int masterWriteFd[2];
	if(pipe(masterWriteFd) < 0){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[CTCPServerException] can not create pipe..");
		return;
	}

	//fork �ӽ��� ����ѭ������Ϣ
	int i = 0;
	for(i = 0 ; i < 1 ;i++){


		int forkPid = -1;
		forkPid=fork();
		if(forkPid==-1){

		}else if(forkPid == 0){

			close(masterWriteFd[1]);
			initRecProcess(socket,clientMainEpollFd,masterWriteFd[0]);

		}else{

			//���ùܵ�
			clientRecProcessWritePipeFd = masterWriteFd[1];
			close(masterWriteFd[0]);

			//��¼���߳�ID
			clientRecProcessId = forkPid;
	
			signal(SIGCHLD, catchTcpClientChildSig);

			//���ö�ʱ������
			timerChecker(timerCallback);
			initTimer();

			atexit(clientProcessExit);
		}
	}
}

PHP_METHOD(CTcpClient,on){
	zval	*callback,
			*saveCallback;
	char	*callback_name,
			*eventName;
	int		eventNameLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sz",&eventName,&eventNameLen,&callback) == FAILURE){
		RETURN_FALSE;
	}

	if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
        efree(callback_name);
		throwClientException("[CTcpClientException] call [CTcpClient->on] the params is not a callback function");
        RETVAL_ZVAL(getThis(),1,0);
        return;
    }

	zval	*eventTable;


	//save to callbackObject
	eventTable = zend_read_property(CTcpClientCe,getThis(),ZEND_STRL("eventTable"), 0 TSRMLS_CC);
	MAKE_STD_ZVAL(saveCallback);
	ZVAL_ZVAL(saveCallback,callback,1,0);
	add_assoc_zval(eventTable,eventName,saveCallback);
	zend_update_property(CTcpClientCe,getThis(),ZEND_STRL("eventTable"),eventTable TSRMLS_CC);

	clientPHPObject = getThis();

    efree(callback_name);
	RETVAL_ZVAL(getThis(),1,0);
}


void createMessage(char *event,char *message,int type,char **stringMessage TSRMLS_DC){

	//������Ϣ
	zval	*messageZval;
	char	*jsonEncoder,
			*base64Encoder;

	MAKE_STD_ZVAL(messageZval);
	array_init(messageZval);

	add_next_index_long(messageZval,type);
	add_next_index_string(messageZval,event,1);
	add_next_index_string(messageZval,message,1);
	
	json_encode(messageZval,&jsonEncoder);
	base64Encode(jsonEncoder,&base64Encoder);
	spprintf(stringMessage,0,"%s%c",base64Encoder,'\n');

	//destroy
	zval_ptr_dtor(&messageZval);
	efree(jsonEncoder);
	efree(base64Encoder);
}


PHP_METHOD(CTcpClient,emit){

	char	*message = NULL,
			*event = NULL;
	int		messageLen = 0,
			eventLen = 0;
	zval	*socket,
			*sendList;
	
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ss",&event,&eventLen,&message,&messageLen) == FAILURE){
		RETURN_FALSE;
	}

	if(event == NULL || message == NULL){
		RETURN_FALSE;
	}

	
	//��¼һ��������Ϣ 
	char *stringMessage;
	createMessage(event,message,MSG_USER,&stringMessage TSRMLS_CC);

	//�������ܵ�
	int status = -1;
	while(1){
		status = write(clientRecProcessWritePipeFd,stringMessage,strlen(stringMessage));
		if(status == strlen(stringMessage)){
			break;
		}
		//����ʧ������������
		usleep(500);
	}

	efree(stringMessage);

	if(status <= 0){
		RETVAL_FALSE;
	}

	RETVAL_TRUE;
}


PHP_METHOD(CSocket,close)
{
	zval *socketId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("socketId"), 0 TSRMLS_CC);
	serverCloseClientSocket(Z_LVAL_P(socketId),getThis());
}

PHP_METHOD(CSocket,client)
{}

PHP_METHOD(CSocket,read)
{
	zval *message = zend_read_property(CSocketCe,getThis(),ZEND_STRL("message"), 0 TSRMLS_CC);
	RETVAL_ZVAL(message,1,0);
}

PHP_METHOD(CSocket,emit)
{
	char	*message,
			*event;
	int		messageLen = 0,
			eventLen = 0;
	zval	*socket,
			*sendList;
	
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ss",&event,&eventLen,&message,&messageLen) == FAILURE){
		throwServerException("[CTcpServerException] call [CTcpServer->emit] params error ,need 2 string");
		RETURN_FALSE;
	}

	//��¼һ��������Ϣ
	char *stringMessage;
	createMessage(event,message,MSG_USER,&stringMessage TSRMLS_CC);

	zval *socketId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("socketId"), 0 TSRMLS_CC);
	zval *socketType = zend_read_property(CSocketCe,getThis(),ZEND_STRL("socketType"), 0 TSRMLS_CC);

	zval *sendMessage;
	MAKE_STD_ZVAL(sendMessage);
	array_init(sendMessage);
	add_index_long(sendMessage,0,Z_LVAL_P(socketId));
	add_index_string(sendMessage,1,stringMessage,0);

	//ͬ����Ϣ����
	if(2 == Z_LVAL_P(socketType)){
		int len = zend_hash_num_elements(Z_ARRVAL_P(clientSendList));
		if(len >= MAX_SENDLIST_LEN){
			zval_ptr_dtor(&sendMessage);
			RETURN_FALSE;
		}
		add_next_index_zval(clientSendList,sendMessage);
		RETVAL_LONG(len+1);
	}

	if(1 == Z_LVAL_P(socketType)){
		
		//�жϿͻ����Ƿ�������
		char socketIndex[100];
		sprintf(socketIndex,"%d_%d",getpid(),Z_LVAL_P(socketId));
		if(IS_ARRAY == Z_TYPE_P(serverSocketList) && zend_hash_exists(Z_ARRVAL_P(serverSocketList),socketIndex,strlen(socketIndex)+1) ){
		}else{
			zval_ptr_dtor(&sendMessage);
			RETURN_FALSE;
		}

		//��ȡ��socket���б�
		zval **thisSocketList;
		if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(serverSendList),Z_LVAL_P(socketId),(void**)&thisSocketList) && IS_ARRAY == Z_TYPE_PP(thisSocketList) ){
			if(zend_hash_num_elements(Z_ARRVAL_PP(thisSocketList)) >= MAX_SENDLIST_LEN ){
				RETURN_FALSE;
			}
		}else{
			//���������򴴽�һ��
			zval *saveArray;
			MAKE_STD_ZVAL(saveArray);
			array_init(saveArray);
			add_index_zval(serverSendList,Z_LVAL_P(socketId),saveArray);
			zend_hash_index_find(Z_ARRVAL_P(serverSendList),Z_LVAL_P(socketId),(void**)&thisSocketList);
		}

		if(IS_ARRAY != Z_TYPE_PP(thisSocketList)){
			RETURN_FALSE;
		}

		//���¶�ȡ
		int n =zend_hash_num_elements(Z_ARRVAL_PP(thisSocketList));
		add_next_index_zval(*thisSocketList,sendMessage);
		RETURN_LONG(n+1);
	}
}


PHP_METHOD(CSocket,getSocketId)
{
	zval *socketId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("socketId"), 0 TSRMLS_CC);
	RETVAL_ZVAL(socketId,1,0);
}

PHP_METHOD(CSocket,getRemoteIp)
{
	zval *remoteIp = zend_read_property(CSocketCe,getThis(),ZEND_STRL("remoteIp"), 0 TSRMLS_CC);
	RETVAL_ZVAL(remoteIp,1,0);
}

PHP_METHOD(CSocket,getConnectTime)
{
	zval *data = zend_read_property(CSocketCe,getThis(),ZEND_STRL("connectTime"), 0 TSRMLS_CC);
	RETVAL_ZVAL(data,1,0);
}

PHP_METHOD(CSocket,getSessionId)
{
	char	*sessionId,
			sess[100];
	zval *socketId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("socketId"), 0 TSRMLS_CC);
	sprintf(sess,"%d_%d",getpid(),Z_LVAL_P(socketId));
	RETVAL_STRING(sess,1);
}

PHP_METHOD(CSocket,getProcessId)
{
	zval *data = zend_read_property(CSocketCe,getThis(),ZEND_STRL("processId"), 0 TSRMLS_CC);
	RETVAL_ZVAL(data,1,0);
}

PHP_METHOD(CSocket,getLastActiveTime)
{
	zval *data = zend_read_property(CSocketCe,getThis(),ZEND_STRL("lastActiveTime"), 0 TSRMLS_CC);
	RETVAL_ZVAL(data,1,0);
}

PHP_METHOD(CSocket,getSocket)
{
	zval	*object,
			**socketInfo,
			**targetSocketId;
	char	*sessionId;
	int		sessionIdLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&sessionId,&sessionIdLen) == FAILURE){
		RETURN_FALSE;
	}

	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(serverSocketList),sessionId,strlen(sessionId)+1,(void**)&socketInfo) && IS_ARRAY == Z_TYPE_PP(socketInfo) &&
		SUCCESS == zend_hash_find(Z_ARRVAL_PP(socketInfo),"socketId",strlen("socketId")+1,(void**)&targetSocketId) && IS_LONG == Z_TYPE_PP(targetSocketId)
	){
	}else{
		char errMessage[512];
		sprintf(errMessage,"[CTcpServerException] call [CTcpServer->getSocket] can not find the socket object by sessionId:%s",sessionId);
		throwServerException(errMessage);
		return;
	}

	//����һ���Լ�
	MAKE_STD_ZVAL(object);
	ZVAL_ZVAL(object,getThis(),1,0);
	zend_update_property_long(CSocketCe,object,ZEND_STRL("socketId"),Z_LVAL_PP(targetSocketId) TSRMLS_CC);
	RETVAL_ZVAL(object,1,1);
}

PHP_METHOD(CSocket,getAllConnection)
{
	if(serverSocketList == NULL){
		array_init(return_value);
		return;
	}

	RETVAL_ZVAL(serverSocketList,1,0);
}

PHP_METHOD(CTcpClient,close)
{
	//clientMainEpollFd clientSocketId
	ClientCloseSocket(clientMainEpollFd,clientSocketId);

	clientStatus = 4;

	kill(clientRecProcessId,SIGKILL);
}

PHP_METHOD(CTcpServer,onError)
{
	zval	*callback,
			*saveHandler;
	char	*callback_name;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&callback) == FAILURE){
		RETURN_FALSE;
	}

	if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
        efree(callback_name);
        RETVAL_ZVAL(getThis(),1,0);
        return;
    }
	efree(callback_name);

	MAKE_STD_ZVAL(serverErrorHandler);
	ZVAL_ZVAL(serverErrorHandler,callback,1,0);

	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CTcpClient,onError){
	zval	*callback,
			*saveHandler;
	char	*callback_name;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&callback) == FAILURE){
		RETURN_FALSE;
	}

	if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
        efree(callback_name);
        RETVAL_ZVAL(getThis(),1,0);
        return;
    }
	efree(callback_name);

	MAKE_STD_ZVAL(clientErrorHandler);
	ZVAL_ZVAL(clientErrorHandler,callback,1,0);

	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CTcpClient,sleep){

	ulong	time = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"l",&time) == FAILURE){
		RETURN_FALSE;
	}

	clientSleepFlag = time;

	while(1){
		if(clientSleepFlag == 0){
			sleep(1);
			break;
		}
	}
}

#endif