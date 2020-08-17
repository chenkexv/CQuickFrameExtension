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
#include <sys/sem.h>
#include <sys/utsname.h>

#define MSG_USER 1
#define MSG_HEART 2

#define MAX_SENDLIST_LEN  65535

//������ȫ�ֱ���
static zval *serverSocketList = NULL;
static zval *serverSessionList = NULL;
static smart_str serverTempBuffer[10240] = {0};
static zval *serverErrorHandler = NULL;
static zval *serverSendList = NULL;
static zval *serverMainObject = NULL;
static int serverListenSocket = 0;
static int serverAccpetLock = 0;
static zval *serverGroup = NULL;
static int serverToGatewaySocket = 0;
static char *serverSessionId = NULL;
static int serverToGatewayStatus = 0; //0 δ���� 1���ӳɹ� 2�Ͽ����ӣ� 3�������� 4�����Ͽ���������
static int serverGatewayLastActiveTime = 0;
static int serverLastCheckStaus = 0;
static int serverEpollFd = 0;
static zval *serverLocalSessionData = NULL;
static zval *serverUidData = NULL;
static zval *serverAsyncList = NULL;	//�������첽��ѯ���¼��б�

//����ȫ�ֱ���
static zval *gatewaySocketList = NULL;		//worker�ļ�¼
static zval *gatewaySessionList = NULL;		//worker�ļ�¼
static zval *gatewayUserSessionList = NULL;
static zval *gatewayUidSessionList = NULL;	//uid��session�Ķ�Ӧ��ϵ
static zval *gatewaySendList = NULL;
static int gatewayLastCheckTime = 0;
static zval *gatewayGroupList = NULL;	//Ⱥ���б�
static zval *gatewaySessionData = NULL;	//���session



//�ͻ���ȫ�ֱ���
static int clientVersion = 1;
static int clientErrNums = 0;
static int clientStatus = 0;	//0 δ���� 1���ӳɹ� 2�Ͽ����ӣ� 3�������� 4�����Ͽ���������
static int clientSocketId = 0;
static char *clientHost = NULL;
static int clientPort = 0;
static double clientLastMessageTime = 0.0;
static int clientRecProcessId = 0;
static int clientMainEpollFd = 0;
static zval *clientPHPObject = NULL;
static zval *clientErrorHandler = NULL;
static int clientRecProcessWritePipeFd = 0;
static int clientRecProcessReadPipeFd = 0;
static zval *clientSendList = NULL;
static char *clientSessionId = NULL;
static int clientStopMasterWrite = 0;
static int clientLastCheckSocket = 0;
static int clientLastHeartbeat = 0;


//zend�෽��
zend_function_entry CTcpGateway_functions[] = {
	PHP_ME(CTcpGateway,__construct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(CTcpGateway,__destruct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(CTcpGateway,getInstance,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(CTcpGateway,bind,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CTcpGateway,listen,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpGateway,on,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpGateway,onData,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpGateway,onError,NULL,ZEND_ACC_PUBLIC )
	{NULL, NULL, NULL}
};

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
	PHP_ME(CTcpServer,gateway,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpServer,sendToSessionId,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CTcpServer,sendToUid,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CTcpServer,getGroup,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CTcpServer,broadcastToGroup,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CTcpServer,broadcast,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CTcpServer,getAllConnection,NULL,ZEND_ACC_PUBLIC)
	



	{NULL, NULL, NULL}
};

zend_function_entry CTcpClient_functions[] = {
	PHP_ME(CTcpClient,__construct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(CTcpClient,__destruct,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(CTcpClient,getInstance,NULL,ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(CTcpClient,connect,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CTcpClient,on,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpClient,onError,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpClient,send,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpClient,close,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpClient,sleep,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CTcpClient,setHeartbeatInterval,NULL,ZEND_ACC_PUBLIC )
	{NULL, NULL, NULL}
};

zend_function_entry CSocket_functions[] = {
	PHP_ME(CSocket,close,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,getClientInfo,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,read,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,send,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,getGroup,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,getSocketId,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,getRemoteIp,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,getConnectTime,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,getSessionId,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,getProcessId,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,getLastActiveTime,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,joinGroup,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,leaveGroup,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,bindUid,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,unBindUid,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,getSession,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,setSession,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,delSession,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CSocket,clearSession,NULL,ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

zend_function_entry CSocketClient_functions[] = {
	PHP_ME(CSocketClient,read,NULL,ZEND_ACC_PUBLIC )
	PHP_ME(CSocketClient,send,NULL,ZEND_ACC_PUBLIC )
	{NULL, NULL, NULL}
};

//ģ�鱻����ʱ
CMYFRAME_REGISTER_CLASS_RUN(CTcpGateway)
{
	zend_class_entry funCe;
	INIT_CLASS_ENTRY(funCe,"CTcpGateway",CTcpGateway_functions);
	CTcpGatewayCe = zend_register_internal_class_ex(&funCe,CControllerCe,NULL TSRMLS_CC);


	zend_declare_property_null(CTcpGatewayCe, ZEND_STRL("instance"),ZEND_ACC_PRIVATE|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_string(CTcpGatewayCe, ZEND_STRL("host"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CTcpGatewayCe, ZEND_STRL("port"),0,ZEND_ACC_PRIVATE TSRMLS_CC);

	
	//dataCallbackObject
	zend_declare_property_null(CTcpGatewayCe, ZEND_STRL("eventTable"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CTcpGatewayCe, ZEND_STRL("eventDefault"),ZEND_ACC_PRIVATE TSRMLS_CC);

	return SUCCESS;
}


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

	zend_declare_property_string(CTcpServerCe, ZEND_STRL("gatewayHost"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CTcpServerCe, ZEND_STRL("gatewayPort"),0,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CTcpServerCe, ZEND_STRL("gatewayUse"),0,ZEND_ACC_PRIVATE TSRMLS_CC);
	
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
	zend_declare_property_long(CTcpClientCe, ZEND_STRL("heartbeatInterval"),120,ZEND_ACC_PRIVATE TSRMLS_CC);

	return SUCCESS;
}

CMYFRAME_REGISTER_CLASS_RUN(CSocket)
{
	zend_class_entry funCe;
	INIT_CLASS_ENTRY(funCe,"CSocket",CSocket_functions);
	CSocketCe = zend_register_internal_class(&funCe TSRMLS_CC);


	zend_declare_property_string(CSocketCe, ZEND_STRL("message"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CSocketCe, ZEND_STRL("socketId"),0,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CSocketCe, ZEND_STRL("client"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CSocketCe, ZEND_STRL("remoteIp"),"0.0.0.0",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_double(CSocketCe, ZEND_STRL("connectTime"),0.00,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_double(CSocketCe, ZEND_STRL("lastActiveTime"),0.00,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CSocketCe, ZEND_STRL("processId"),0,ZEND_ACC_PRIVATE TSRMLS_CC);


	//socket��������  1Ϊ������ʹ�õĶ���  2Ϊ�ͻ���ʹ�õĶ���
	zend_declare_property_long(CSocketCe, ZEND_STRL("socketType"),0,ZEND_ACC_PRIVATE TSRMLS_CC);
	return SUCCESS;
}

CMYFRAME_REGISTER_CLASS_RUN(CSocketClient)
{
	zend_class_entry funCe;
	INIT_CLASS_ENTRY(funCe,"CSocketClient",CSocketClient_functions);
	CSocketClientCe = zend_register_internal_class(&funCe TSRMLS_CC);

	zend_declare_property_string(CSocketClientCe, ZEND_STRL("message"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CSocketClientCe, ZEND_STRL("socketId"),0,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CSocketClientCe, ZEND_STRL("socketType"),0,ZEND_ACC_PRIVATE TSRMLS_CC);

	return SUCCESS;
}

void writeLogs(char *format, ...){
	
	va_list args;
    int ret;
    char *buffer;
    int size;

    va_start(args, format);
    size = vspprintf(&buffer, 0, format, args);

	char	*showDate;
	php_date("Y-m-d h:i:s",&showDate);
   
	php_printf("[%s]=>%s",showDate,buffer);
	
	efree(showDate);
    efree(buffer);
    va_end(args);
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
void createWorkProcess(int forkNum,int listenfd,int lock,zval *object,int isFrist TSRMLS_DC);
void createMessage(char *event,char *message,int type,char **stringMessage,int useType TSRMLS_DC);

void checkSocketStatus(){


	if(clientStatus == 2){

		//��socket״̬Ϊ �Ͽ�ʱ����Ϊ��������
		clientStatus == 3;

		//����socket
		int restetStatus = resetSocketConnect();

	}
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

void destoryTimer(){
	struct itimerval value;
	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = 0;
	value.it_interval = value.it_value;
	setitimer(ITIMER_REAL, &value, NULL);
}


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
		writeLogs("[CTcpServer] The master check child status,now alive [%d],need keep num[%d],need create[%d]\n",aliveNum,need,needCreate);
		
		createWorkProcess(needCreate,serverListenSocket,serverAccpetLock,serverMainObject,2 TSRMLS_CC);
	}
	

}

void catchTcpChildSig(int sig){
	writeLogs("[CTcpServer] [%d-%d] Receive a child process exit signal [%d]\n",getppid(),getpid(),sig);
	int endPid = wait(NULL);
	writeLogs("[CTcpServer] The process for determining the unexpected termination is [%d]\n",endPid);

	zval	*pidList = zend_read_property(CTcpServerCe,serverMainObject,ZEND_STRL("pidList"),1 TSRMLS_CC);
	if(IS_ARRAY == Z_TYPE_P(pidList) && zend_hash_index_exists(Z_ARRVAL_P(pidList),endPid)){
		zend_hash_index_del(Z_ARRVAL_P(pidList),endPid);
	}
	checkTcpServerWorkerStatus(serverMainObject TSRMLS_CC);
}

//�����ط�����������Ϣ
void sendToGateway(char *stringMessage){

	zval *sendMessage;
	MAKE_STD_ZVAL(sendMessage);
	array_init(sendMessage);
	add_index_long(sendMessage,0,serverToGatewaySocket);
	add_index_string(sendMessage,1,stringMessage,1);

	//�жϿͻ����Ƿ�������
	char socketIndex[100];
	sprintf(socketIndex,"%d_%d",getpid(),serverToGatewaySocket);

	//��ȡ��socket���б�
	zval **thisSocketList;
	if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(serverSendList),serverToGatewaySocket,(void**)&thisSocketList) && IS_ARRAY == Z_TYPE_PP(thisSocketList) ){
	}else{
		//���������򴴽�һ��
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		add_index_zval(serverSendList,serverToGatewaySocket,saveArray);
		zend_hash_index_find(Z_ARRVAL_P(serverSendList),serverToGatewaySocket,(void**)&thisSocketList);
	}

	add_next_index_zval(*thisSocketList,sendMessage);
}

//��������ӽ���
void createWorkProcess(int forkNum,int listenfd,int lock,zval *object,int isFrist TSRMLS_DC){

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

			if(i == forkNum - 1 && isFrist == 1){

				while(1){

					//ÿ30����һ���ӽ���״̬
					sleep(30);
					checkTcpServerWorkerStatus(object TSRMLS_CC);
				}

			}

		}
	}

}

void timerCallback();
void catchTcpClientChildSig(int sig){
	writeLogs("[CTcpClient] [%d-%d] Receive a child process exit signal [%d]\n",getppid(),getpid(),sig);
	int endPid = wait(NULL);
	writeLogs("[CTcpClient] The process for determining the unexpected termination is [%d]\n",endPid);
	if(endPid == clientRecProcessId){
		if(clientStatus != 4){
			clientStatus = 2;

			//��װ��ʱ��
			timerChecker(timerCallback);
			initTimer();

			//����socket
			checkSocketStatus();
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

int server_sendMessage(int fd,char *stringMessage){
	
	//�жϿͻ����Ƿ�������
	char socketIndex[100];
	sprintf(socketIndex,"%d_%d",getpid(),fd);
	if(IS_ARRAY == Z_TYPE_P(serverSocketList) && zend_hash_exists(Z_ARRVAL_P(serverSocketList),socketIndex,strlen(socketIndex)+1) ){
	}else{
		return -1;
	}

	//��ȡ��socket���б�
	zval **thisSocketList;
	if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(serverSendList),fd,(void**)&thisSocketList) && IS_ARRAY == Z_TYPE_PP(thisSocketList) ){
	}else{
		//���������򴴽�һ��
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		add_index_zval(serverSendList,fd,saveArray);
		zend_hash_index_find(Z_ARRVAL_P(serverSendList),fd,(void**)&thisSocketList);
	}

	if(IS_ARRAY != Z_TYPE_PP(thisSocketList)){
		return -3;
	}

	zval *sendMessage;
	MAKE_STD_ZVAL(sendMessage);
	array_init(sendMessage);
	add_index_long(sendMessage,0,fd);
	char *sendStringMessage;
	spprintf(&sendStringMessage,0,"%s%c",stringMessage,'\n');
	add_index_string(sendMessage,1,sendStringMessage,0);

	//д��һ����Ϣ
	add_next_index_zval(*thisSocketList,sendMessage);
	return 1;
}

int processServerSystemMessage(int fd,zval *jsonDecoder,zval *object TSRMLS_DC){

	zval **messageEvent;

	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),1,(void**)&messageEvent);

	//�����¼�
	if(strcmp(Z_STRVAL_PP(messageEvent),"connect") == 0){


		//���Ϳͻ�����Ϣ Ҫ���ϱ��ͻ��˻�����Ϣ  [2,"connect",""]
		server_sendMessage(fd,"WzIsImNvbm5lY3QiLCIiXQ==");

		return 1;
	}

	
	//����ϵͳ�¼�
	if(strcmp(Z_STRVAL_PP(messageEvent),"clientHeartbeat") == 0){
		
		//����socket��󼤻�ʱ��
		zval	**socketInfo;
		char	socketIndex[100];
		sprintf(socketIndex,"%d_%d",getpid(),fd);
		if(SUCCESS == zend_hash_find(Z_ARRVAL_P(serverSocketList),socketIndex,strlen(socketIndex)+1,(void**)&socketInfo) && IS_ARRAY == Z_TYPE_PP(socketInfo)){
		}else{
			return -1;
		}

		zval *timeZval;
		microtime(&timeZval);
		add_assoc_double(*socketInfo,"lastActiveTime",Z_DVAL_P(timeZval));
		zval_ptr_dtor(&timeZval);
	}

	//����ʱ�ͻ����ϱ��Ļ�����Ϣ
	if(strcmp(Z_STRVAL_PP(messageEvent),"clientReport") == 0){

			//����socket��󼤻�ʱ��
		zval	**socketInfo;
		char	socketIndex[100];
		sprintf(socketIndex,"%d_%d",getpid(),fd);
		if(SUCCESS == zend_hash_find(Z_ARRVAL_P(serverSocketList),socketIndex,strlen(socketIndex)+1,(void**)&socketInfo) && IS_ARRAY == Z_TYPE_PP(socketInfo)){
		}else{
			return -1;
		}

		//����2
		zval	**messageContent,
				*messageArr,
				**sessionIdZval;

		zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),2,(void**)&messageContent);
		json_decode(Z_STRVAL_PP(messageContent),&messageArr);

		char *sessionId;
		if(SUCCESS == zend_hash_find(Z_ARRVAL_P(messageArr),"sessionId",strlen("sessionId")+1,(void**)&sessionIdZval) ){
			sessionId = estrdup(Z_STRVAL_PP(sessionIdZval));
		}else{
			sessionId = estrdup("unknow");
		}
		add_assoc_string(*socketInfo,"sessionId",sessionId,1);
		if(IS_ARRAY == Z_TYPE_P(messageArr)){
			add_assoc_zval(*socketInfo,"client",messageArr);
		}else{
			zval_ptr_dtor(&messageArr);
			zval *clientInfo;
			MAKE_STD_ZVAL(clientInfo);
			array_init(clientInfo);
			add_assoc_zval(*socketInfo,"client",clientInfo);
		}

		//��¼serverSessionList
		add_assoc_long(serverSessionList,sessionId,fd);

		//������Ϣ�����ط�����
		char	acceptMessage[200],
				*gatewayMessage;
		sprintf(acceptMessage,"{\"sessionId\":\"%s\"}",sessionId);
		createMessage("connect",acceptMessage,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
		sendToGateway(gatewayMessage);
		efree(gatewayMessage);
		efree(sessionId);


		//���Ϳͻ�����Ϣ �������ӳɹ�
		server_sendMessage(fd,"WzIsImNvbm5lY3RlZCIsIiJd");

		//�յ��ͻ��˻ظ������connect�¼� WzEsImNvbm5lY3QiLCIiXQ==  [1,"connect",""]
		return processMessage(fd,"WzEsImNvbm5lY3QiLCIiXQ==",object TSRMLS_CC);
	}

}

int processMessage(int fd,char *thisMessage,zval *object TSRMLS_DC){

	char	*base64Decoder;
	zval	*jsonDecoder,
			*eventTable,
			*eventDefault,
			**messageEvent,
			**eventCallback,
			**stringMessage,
			**messageType;

	base64Decode(thisMessage,&base64Decoder);
	json_decode(base64Decoder,&jsonDecoder);

	if(zend_hash_num_elements(Z_ARRVAL_P(jsonDecoder)) < 3){
		efree(base64Decoder);
		zval_ptr_dtor(&jsonDecoder);
		char *errorMessage;
		spprintf(&errorMessage,0,"some message parse error : %s",thisMessage);
		throwServerException(errorMessage);
		efree(errorMessage);
		return -1;
	}

	//��ǰ���¼�����
	if(	SUCCESS == zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),0,(void**)&messageType) &&
		SUCCESS == zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),1,(void**)&messageEvent) &&
		SUCCESS == zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),2,(void**)&stringMessage) ){
	}else{
		efree(base64Decoder);
		zval_ptr_dtor(&jsonDecoder);
		return -2;
	}

	if(IS_LONG == Z_TYPE_PP(messageType) && 2 == Z_LVAL_PP(messageType)){
		processServerSystemMessage(fd,jsonDecoder,object TSRMLS_CC);
		efree(base64Decoder);
		zval_ptr_dtor(&jsonDecoder);
		return 1;
	}

	//����Ƿ����ָ����event
	eventTable = zend_read_property(CTcpServerCe,object,ZEND_STRL("eventTable"), 0 TSRMLS_CC);

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
			if(SUCCESS == zend_hash_find(Z_ARRVAL_PP(socketInfo),"remoteIp",strlen("remoteIp")+1,(void**)&thisInfoItem) && IS_STRING == Z_TYPE_PP(thisInfoItem) ){
				char *ip;
				ip = estrdup(Z_STRVAL_PP(thisInfoItem));
				zend_update_property_string(CSocketCe,callParams,ZEND_STRL("remoteIp"),ip TSRMLS_CC);
				efree(ip);
			}
			if(SUCCESS == zend_hash_find(Z_ARRVAL_PP(socketInfo),"connectTime",strlen("connectTime")+1,(void**)&thisInfoItem) && IS_DOUBLE == Z_TYPE_PP(thisInfoItem) ){
				zend_update_property_double(CSocketCe,callParams,ZEND_STRL("connectTime"),Z_DVAL_PP(thisInfoItem) TSRMLS_CC);
			}
			if(SUCCESS == zend_hash_find(Z_ARRVAL_PP(socketInfo),"client",strlen("client")+1,(void**)&thisInfoItem) && IS_ARRAY == Z_TYPE_PP(thisInfoItem) ){
				zval *saveItem;
				MAKE_STD_ZVAL(saveItem);
				ZVAL_ZVAL(saveItem,*thisInfoItem,1,0);
				zend_update_property(CSocketCe,callParams,ZEND_STRL("client"),saveItem TSRMLS_CC);
				zval_ptr_dtor(&saveItem);
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

void serverClearLocalData(char *sessionId){
	
	if(zend_hash_exists(Z_ARRVAL_P(serverGroup),sessionId,strlen(sessionId)+1)){
		zend_hash_del(Z_ARRVAL_P(serverGroup),sessionId,strlen(sessionId)+1);
	}

	if(zend_hash_exists(Z_ARRVAL_P(serverSessionList),sessionId,strlen(sessionId)+1)){
		zend_hash_del(Z_ARRVAL_P(serverSessionList),sessionId,strlen(sessionId)+1);
	}

	//������uidsession
	int		i,h;
	zval	**thisSession;
	char	*key;
	ulong	uKey;
	h = zend_hash_num_elements(Z_ARRVAL_P(serverUidData));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(serverUidData));
	for(i = 0 ; i < h; i++){
		if(SUCCESS == zend_hash_get_current_data(Z_ARRVAL_P(serverUidData),(void**)&thisSession) && IS_STRING == Z_TYPE_PP(thisSession) && strcmp(Z_STRVAL_PP(thisSession),sessionId) == 0 ){
			zend_hash_get_current_key(Z_ARRVAL_P(serverUidData),&key,&uKey,0);
			zend_hash_del(Z_ARRVAL_P(serverUidData),key,strlen(key)+1);
		}
		zend_hash_move_forward(Z_ARRVAL_P(serverUidData));
	}

}

//�������رտͻ�������
void serverCloseClientSocket(int socketFd,zval *object){
	
	//���socket�б�
	char socketIndex[100];
	sprintf(socketIndex,"%d_%d",getpid(),socketFd);

	if(IS_ARRAY == Z_TYPE_P(serverSocketList) && zend_hash_exists(Z_ARRVAL_P(serverSocketList),socketIndex,strlen(socketIndex)+1) ){
		//�����ر��¼�
		//WzEsImRpc2Nvbm5lY3QiLCIiXQ==  [1,"disconnect",""]
		processMessage(socketFd,"WzEsImRpc2Nvbm5lY3QiLCIiXQ==",object TSRMLS_CC);

		//֪ͨ����
		zval	**thisSocketInfo,
				**sessionIdZval;

		if( SUCCESS == zend_hash_find(Z_ARRVAL_P(serverSocketList),socketIndex,strlen(socketIndex)+1,(void**)&thisSocketInfo) && IS_ARRAY == Z_TYPE_PP(thisSocketInfo) &&
			SUCCESS == zend_hash_find(Z_ARRVAL_PP(thisSocketInfo),"sessionId",strlen("sessionId")+1,(void**)&sessionIdZval) && IS_STRING == Z_TYPE_PP(sessionIdZval)
		){
			char	acceptMessage[200],
					*gatewayMessage;

			//�����ص�Ⱥ����Ϣ session����
			serverClearLocalData(Z_STRVAL_PP(sessionIdZval));

			sprintf(acceptMessage,"{\"sessionId\":\"%s\"}",Z_STRVAL_PP(sessionIdZval));
			createMessage("disconnect",acceptMessage,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
			sendToGateway(gatewayMessage);
			efree(gatewayMessage);
		}


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

//������worker��ת����Ϣ
void serverBroadcastMessage(char *message,zval *object TSRMLS_DC){

	//��ǰ����socket
	int		i,h;
	zval	**thisSocket,
			**socketId,
			*sendMessage;

	h = zend_hash_num_elements(Z_ARRVAL_P(serverSocketList));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(serverSocketList));
	for(i = 0 ; i < h ; i++){
		zend_hash_get_current_data(Z_ARRVAL_P(serverSocketList),(void**)&thisSocket);

		if(IS_ARRAY == Z_TYPE_PP(thisSocket) && SUCCESS == zend_hash_find(Z_ARRVAL_PP(thisSocket),"socketId",strlen("socketId")+1,(void**)&socketId) && IS_LONG == Z_TYPE_PP(socketId) ){
		}else{
			zend_hash_move_forward(Z_ARRVAL_P(serverSocketList));
			continue;
		}

		zval **thisSocketList;
		if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(serverSendList),Z_LVAL_PP(socketId),(void**)&thisSocketList) && IS_ARRAY == Z_TYPE_PP(thisSocketList) ){
		}else{
			//���������򴴽�һ��
			zval *saveArray;
			MAKE_STD_ZVAL(saveArray);
			array_init(saveArray);
			add_index_zval(serverSendList,Z_LVAL_PP(socketId),saveArray);
			zend_hash_index_find(Z_ARRVAL_P(serverSendList),Z_LVAL_PP(socketId),(void**)&thisSocketList);
		}

		//д��һ����Ϣ
		zval *sendMessage;
		MAKE_STD_ZVAL(sendMessage);
		array_init(sendMessage);
		add_index_long(sendMessage,0,Z_LVAL_PP(socketId));
		char	*baseMessage,
				*sendMessageString;
		base64Encode(message,&baseMessage);
		spprintf(&sendMessageString,0,"%s%c",baseMessage,'\n');
		efree(baseMessage);
		add_index_string(sendMessage,1,sendMessageString,0);
		add_next_index_zval(*thisSocketList,sendMessage);


		zend_hash_move_forward(Z_ARRVAL_P(serverSocketList));

	}


}

void triggerEventBack(char *message,zval *object TSRMLS_DC){

	zval	*jsonDecoder,
			**eventId,
			**data,
			**eventCallback;
	
	json_decode(message,&jsonDecoder);

	if(	SUCCESS == zend_hash_find(Z_ARRVAL_P(jsonDecoder),"eventId",8,(void**)&eventId) && IS_STRING == Z_TYPE_PP(eventId) && SUCCESS == zend_hash_find(Z_ARRVAL_P(jsonDecoder),"data",5,(void**)&data)){
	}else{
		zval_ptr_dtor(&jsonDecoder);
		writeLogs("[CTcpServer] can not parse event message:%s\n",message);
		return;
	}


	//Ѱ�Ҵ��¼��Ļص�����
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(serverAsyncList),Z_STRVAL_PP(eventId),strlen(Z_STRVAL_PP(eventId))+1,(void**)&eventCallback) && IS_OBJECT == Z_TYPE_PP(eventCallback) ){
	}else{
		zval_ptr_dtor(&jsonDecoder);
		return;
	}



	//�����¼��ص�
	char	*callback_name = NULL;
	zval	*callback;
	callback = *eventCallback;
	if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
		efree(callback_name);
		zval_ptr_dtor(&jsonDecoder);
		return;
	}

	zval	constructVal,
			contruReturn,
			*paramsList[1];

	INIT_ZVAL(constructVal);
	ZVAL_STRING(&constructVal,callback_name,0);
	MAKE_STD_ZVAL(paramsList[0]);
	ZVAL_ZVAL(paramsList[0],*data,1,0);
	int callStatus = call_user_function(NULL, &callback, &constructVal, &contruReturn,1, paramsList TSRMLS_CC);
	zval_ptr_dtor(&paramsList[0]);
	zval_dtor(&contruReturn);


	//ɾ���¼�
	zend_hash_del(Z_ARRVAL_P(serverAsyncList),Z_STRVAL_PP(eventId),strlen(Z_STRVAL_PP(eventId))+1);


	efree(callback_name);
	zval_ptr_dtor(&jsonDecoder);

}

void triggerForwordMessage(char *messageContent,zval *object TSRMLS_DC){


	zval	*message,
			**toUid,
			**toUidSession,
			**socketFd,
			**messageContentZval,
			**event;

	json_decode(messageContent,&message);

	if( SUCCESS == zend_hash_find(Z_ARRVAL_P(message),"toUid",strlen("toUid")+1,(void**)&toUid) && IS_STRING == Z_TYPE_PP(toUid) && 
	   	SUCCESS == zend_hash_find(Z_ARRVAL_P(message),"event",strlen("event")+1,(void**)&event) && IS_STRING == Z_TYPE_PP(event) && 
		SUCCESS == zend_hash_find(Z_ARRVAL_P(message),"message",strlen("message")+1,(void**)&messageContentZval) ){
	}else{
		zval_ptr_dtor(&message);
		return;
	}

	//Ѱ�ҵ�ǰtoUid��Ӧ��sessionId
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(serverUidData),Z_STRVAL_PP(toUid),strlen(Z_STRVAL_PP(toUid))+1,(void**)&toUidSession) && IS_STRING == Z_TYPE_PP(toUidSession) ){
	}else{
		zval_ptr_dtor(&message);
		return;
	}
	
	//����sessionID��socketfd
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(serverSessionList),Z_STRVAL_PP(toUidSession),strlen(Z_STRVAL_PP(toUidSession))+1,(void**)&socketFd) && IS_LONG == Z_TYPE_PP(socketFd) ){
	}else{
		zval_ptr_dtor(&message);
		return;
	}

	zval	*messageZval,
			*saveMessageContent;
	char	*jsonEncoder,
			*base64Encoder,
			*sendString;

	MAKE_STD_ZVAL(messageZval);
	array_init(messageZval);
	MAKE_STD_ZVAL(saveMessageContent);
	ZVAL_ZVAL(saveMessageContent,*messageContentZval,1,0);


	add_next_index_long(messageZval,1);
	add_next_index_string(messageZval,Z_STRVAL_PP(event),1);
	add_next_index_zval(messageZval,saveMessageContent);


	json_encode(messageZval,&jsonEncoder);
	base64Encode(jsonEncoder,&base64Encoder);
	spprintf(&sendString,0,"%s%c",base64Encoder,'\n');


	zval **thisSocketList;
	if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(serverSendList),Z_LVAL_PP(socketFd),(void**)&thisSocketList) && IS_ARRAY == Z_TYPE_PP(thisSocketList) ){
	}else{
		//���������򴴽�һ��
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		add_index_zval(serverSendList,Z_LVAL_PP(socketFd),saveArray);
		zend_hash_index_find(Z_ARRVAL_P(serverSendList),Z_LVAL_PP(socketFd),(void**)&thisSocketList);
	}

	//β������һ����Ϣ
	zval *sendMessage;
	MAKE_STD_ZVAL(sendMessage);
	array_init(sendMessage);
	add_index_long(sendMessage,0,Z_LVAL_PP(socketFd));
	add_index_string(sendMessage,1,sendString,1);
	add_next_index_zval(*thisSocketList,sendMessage);


	efree(sendString);
	efree(base64Encoder);
	efree(jsonEncoder);
	zval_ptr_dtor(&messageZval);
	zval_ptr_dtor(&message);

}

void serverProcessGatewayMessage(int fd,char *thisMessage,zval *object TSRMLS_DC){

	//��¼�����ػ��������ʱ��
	serverGatewayLastActiveTime = getMicrotime();

	//������Ϣ
	char	*base64Decoder;
	zval	*jsonDecoder,
			**messageType,
			**messageContent,
			**senderSessionId;

	base64Decode(thisMessage,&base64Decoder);
	json_decode(base64Decoder,&jsonDecoder);

	if(IS_ARRAY != Z_TYPE_P(jsonDecoder) || zend_hash_num_elements(Z_ARRVAL_P(jsonDecoder)) != 4){
		zval_ptr_dtor(&jsonDecoder);
		efree(base64Decoder);
		return;
	}


	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),1,(void**)&messageType);
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),2,(void**)&messageContent);
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),3,(void**)&senderSessionId);

	//����Ҫ��ת����Ϣ
	if(strcmp(Z_STRVAL_PP(messageType),"broadcast") == 0){
		serverBroadcastMessage(Z_STRVAL_PP(messageContent),object TSRMLS_CC);

	}

	//����PHP��ص��¼�
	if(	strcmp(Z_STRVAL_PP(messageType),"getAllConnection") == 0 ){
		triggerEventBack(Z_STRVAL_PP(messageContent),object TSRMLS_CC);
	}


	//ÿ��worker��¼һ��group��¼
	if(	strcmp(Z_STRVAL_PP(messageType),"joinGroup") == 0){
		triggerEventBack(Z_STRVAL_PP(messageContent),object TSRMLS_CC);
	}

	//���û�
	if(	strcmp(Z_STRVAL_PP(messageType),"bindUid") == 0){
		triggerEventBack(Z_STRVAL_PP(messageContent),object TSRMLS_CC);
	}

	//ת����Ϣ forwardMessage
	if(	strcmp(Z_STRVAL_PP(messageType),"forwardMessage") == 0){
		triggerForwordMessage(Z_STRVAL_PP(messageContent),object TSRMLS_CC);
	}

	//����ת����Ϣ�ص�
	if(	strcmp(Z_STRVAL_PP(messageType),"forwardMessageSuccess") == 0){
		triggerEventBack(Z_STRVAL_PP(messageContent),object TSRMLS_CC);
	}



	zval_ptr_dtor(&jsonDecoder);
	efree(base64Decoder);
}

void serverCloseGatewaySocket(int fd,zval *object){

	writeLogs("[CTcpServer] woker[%d] disconnect from gateway ...\n",getpid());

	//�ر�socket
	close(fd);

	//ɾ�����۵���Ϣ
	zend_hash_index_del(Z_ARRVAL_P(serverSendList),serverToGatewaySocket);
	serverToGatewaySocket = 0;
	serverToGatewayStatus = 2;		//��Ϊ�ѶϿ�����


	//�������¼������״̬
	checkGatewayStatus();
}

int handlerTCPEvents(int epfd,struct epoll_event revs[],int num,int listen_sock,int semid,int isGotLock,zval *object TSRMLS_CC)
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
			add_assoc_long(remoteInfo,"socketId",new_sock);
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

		//������ͨ�ŵ���Ϣ
		if( fd == serverToGatewaySocket && (revs[i].events & EPOLLIN) ){

			char		buf[2],
						*thisMessage;
			int			readLen = 0,k;

			smart_str	tempBuffer[10240] = {0};

			while(1){
				errno = 0;
				readLen = read(fd,buf,sizeof(buf)-1);

				if(readLen <= 0){
					if(readLen == 0){

						//�ͷ�Bbuffer
						smart_str_0(&tempBuffer[fd]);
						smart_str_free(&tempBuffer[fd]);

						epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
						serverCloseGatewaySocket(fd,object);
						
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
					serverProcessGatewayMessage(fd,thisMessage,object TSRMLS_CC);
					efree(thisMessage);
					break;
				}	
			}

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
				
				struct sembuf tryrelease;
				tryrelease.sem_num = 0;
				tryrelease.sem_op = 1;
				tryrelease.sem_flg = SEM_UNDO|IPC_NOWAIT;
				semop(semid , &tryrelease , 1);
				
				nowIsGetLock = 0;
			}

			while(1){
				errno = 0;
				readLen = read(fd,buf,sizeof(buf)-1);

				if(readLen <= 0){
					if(readLen == 0){

						//�ͷ�buffer
						smart_str_0(&tempBuffer[fd]);
						smart_str_free(&tempBuffer[fd]);

						epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
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


		if(SUCCESS == zend_hash_get_current_data(Z_ARRVAL_P(serverSendList),(void**)&socketMessage) && IS_ARRAY == Z_TYPE_PP(socketMessage)){
			zend_hash_internal_pointer_reset(Z_ARRVAL_PP(socketMessage));
			k = zend_hash_num_elements(Z_ARRVAL_PP(socketMessage));
			for(j = 0 ; j < k ;j++){
				zend_hash_get_current_key(Z_ARRVAL_PP(socketMessage),&key,&uKey,0);

				if(SUCCESS == zend_hash_get_current_data(Z_ARRVAL_PP(socketMessage),(void**)&thisMessage) && IS_ARRAY == Z_TYPE_PP(thisMessage)){
				}else{
					zend_hash_move_forward(Z_ARRVAL_PP(socketMessage));
					zend_hash_index_del(Z_ARRVAL_PP(socketMessage),uKey);
					continue;
				}


				if(SUCCESS == zend_hash_index_find(Z_ARRVAL_PP(thisMessage),0,(void**)&toSocket) && IS_LONG == Z_TYPE_PP(toSocket) &&
				   SUCCESS == zend_hash_index_find(Z_ARRVAL_PP(thisMessage),1,(void**)&messageContent) && IS_STRING == Z_TYPE_PP(messageContent)){
				}else{
					zend_hash_move_forward(Z_ARRVAL_PP(socketMessage));
					zend_hash_index_del(Z_ARRVAL_PP(socketMessage),uKey);
					continue;
				}

				errno = 0;
				int status = write(Z_LVAL_PP(toSocket),Z_STRVAL_PP(messageContent),Z_STRLEN_PP(messageContent));

				//д��ʧ��
				if(status <= 0){	
					if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
						break;
					}else{
						
						//�����ط���ʧ��ʱ 
						if(Z_LVAL_PP(toSocket) == serverToGatewaySocket){
							serverCloseGatewaySocket(Z_LVAL_PP(toSocket),object);
							break;
						}

						//�ͻ����ѹر� �����ص�socket
						serverCloseClientSocket(Z_LVAL_PP(toSocket),object);
						break;
					}
				}
				
				//�Ƴ���Ϣ
				zend_hash_move_forward(Z_ARRVAL_PP(socketMessage));
				zend_hash_index_del(Z_ARRVAL_PP(socketMessage),uKey);
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

void tcpClienthandleMaster_signal(int signo){
	//�յ��������Ƴ����ź�
	if (signo == SIGHUP){  
		zend_bailout();
	}
}

//����ǰworker��������ϱ���gateway
void reportWorkerUser(){

	int		i,h;
	zval	**thisSocket,
			**sessionId,
			**clientInfo;

	if(serverSocketList == NULL){
		return;
	}

	h = zend_hash_num_elements(Z_ARRVAL_P(serverSocketList));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(serverSocketList));


	for(i = 0 ; i < h ; i++){

		zend_hash_get_current_data(Z_ARRVAL_P(serverSocketList),(void**)&thisSocket);

		if( SUCCESS == zend_hash_find(Z_ARRVAL_PP(thisSocket),"client",strlen("client")+1,(void**)&clientInfo) && IS_ARRAY == Z_TYPE_PP(clientInfo) &&  
			SUCCESS == zend_hash_find(Z_ARRVAL_PP(clientInfo),"sessionId",strlen("sessionId")+1,(void**)&sessionId) && IS_STRING == Z_TYPE_PP(sessionId) )
		{

			//�����ϱ���gateway
			char	acceptMessage[200],
					*gatewayMessage;
			sprintf(acceptMessage,"{\"sessionId\":\"%s\"}",Z_STRVAL_PP(sessionId));
			createMessage("connect",acceptMessage,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
			sendToGateway(gatewayMessage);
			efree(gatewayMessage);
		}


		zend_hash_move_forward(Z_ARRVAL_P(serverSocketList));
	}


	//�ϱ����ؼ�¼�����Ⱥ��
	char	*key;
	ulong	uKey;
	zval	**groupName;
	h = zend_hash_num_elements(Z_ARRVAL_P(serverGroup));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(serverGroup));

	for(i = 0 ; i < h ; i++){
		zend_hash_get_current_key(Z_ARRVAL_P(serverGroup),&key,&uKey,0);
		zend_hash_get_current_data(Z_ARRVAL_P(serverGroup),(void**)&groupName);

			char	*gatewayMessage,
					eventCon[100];
			sprintf(eventCon,"{\"eventId\":\"none\",\"group\":\"%s\",\"clientSessionId\":\"%s\"}",Z_STRVAL_PP(groupName),key);
			createMessage("joinGroup",eventCon,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
			sendToGateway(gatewayMessage);
			efree(gatewayMessage);
		zend_hash_move_forward(Z_ARRVAL_P(serverGroup));
	}


	//�ϱ����ؼ�¼��uid-session�б�
	h = zend_hash_num_elements(Z_ARRVAL_P(serverUidData));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(serverUidData));
	for(i = 0 ; i < h ; i++){
		zend_hash_get_current_key(Z_ARRVAL_P(serverUidData),&key,&uKey,0);
		zend_hash_get_current_data(Z_ARRVAL_P(serverUidData),(void**)&sessionId);
			char	*gatewayMessage,
			eventCon[100];
			sprintf(eventCon,"{\"eventId\":\"none\",\"uid\":\"%s\",\"clientSessionId\":\"%s\"}",key,Z_STRVAL_PP(sessionId));
			createMessage("bindUid",eventCon,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
			sendToGateway(gatewayMessage);
			efree(gatewayMessage);

		zend_hash_move_forward(Z_ARRVAL_P(serverUidData));
	}




}

//���������ط�����
int serverConnectToGateway(zval *object TSRMLS_DC){

	zval *useZval = zend_read_property(CTcpServerCe,object,ZEND_STRL("gatewayUse"), 0 TSRMLS_CC);
	if(0 == Z_LVAL_P(useZval)){
		return 0;
	}

	zval *hostZval = zend_read_property(CTcpServerCe,object,ZEND_STRL("gatewayHost"), 0 TSRMLS_CC);
	zval *portZval = zend_read_property(CTcpServerCe,object,ZEND_STRL("gatewayPort"), 0 TSRMLS_CC);

	char  *host;
	int	  port;

	host = Z_STRVAL_P(hostZval);
	port = Z_LVAL_P(portZval);

	int socket = connectServerPort(host,port TSRMLS_CC);
	if(socket < 0){
		serverToGatewayStatus = 2;
		writeLogs("[FatalException] [%d] TCP Server can connect to gateway server %s:%d ,errorno : %d \n",getpid(),host,port,socket);
	}

	if(socket > 0){
		serverToGatewaySocket = socket;
		serverToGatewayStatus = 1;
		writeLogs("[CTcpServer] worker[%d] Successfully connected to gateway server ...\n",getpid());
		registerTcpWorker();
		serverGatewayLastActiveTime = getMicrotime();

		//�ϱ���ǰworker�����߼�¼
		reportWorkerUser();

		//����fd����epoll
		struct epoll_event  ev2;
		ev2.events = EPOLLIN; 
		ev2.data.fd = serverToGatewaySocket;
		epoll_ctl(serverEpollFd,EPOLL_CTL_ADD,serverToGatewaySocket,&ev2);

	}

	return socket;
}

//������worker���������ط�����ע��
int registerTcpWorker(){

	if(serverToGatewaySocket <= 0){
		return -1;
	}

	if(serverSessionId == NULL){
		char *sessionId;
		getSessionID(&sessionId);
		serverSessionId = sessionId;
	}

	serverToGatewayStatus = 1;

	char	*stringMessage;
	createMessage("serverRegister","[]",MSG_USER,&stringMessage,3 TSRMLS_CC);

	sendToGateway(stringMessage);
	efree(stringMessage);

	return 1;
}

//������ط�����״̬
int checkGatewayStatus(){
	
	//�������ӻ��������Ͽ�ʱ ������������
	if(serverToGatewayStatus != 2){
		return 1;
	}

	//��������
	TSRMLS_FETCH();
	serverToGatewayStatus = 3;	//����������
	serverToGatewaySocket = serverConnectToGateway(serverMainObject TSRMLS_CC);
	if(serverToGatewaySocket > 0){
		serverToGatewayStatus = 1;
	}else{
		serverToGatewayStatus = 2;
	}
}

void checkGatewayHeartbeat(){

	
	int timenow = getMicrotime();



	if(serverToGatewayStatus == 1 && serverGatewayLastActiveTime > 0 && timenow - serverGatewayLastActiveTime >= 30){
		writeLogs("[CTcpServer] The interaction with the gateway has taken a long time, sending heartbeat packets ...\n");
		char *gatewayMessage;
		createMessage("heartbeat","[]",MSG_USER,&gatewayMessage,3 TSRMLS_CC);
		sendToGateway(gatewayMessage);
		efree(gatewayMessage);
		serverGatewayLastActiveTime = timenow;
	}

}

void serverTimerCallback(){

	//��ȡ��ǰʱ��
	int timestamp = getMicrotime();

	//�����ϴμ�����Ӵ���10��ʱ ���������֮���״̬
	if(timestamp - serverLastCheckStaus >= 10){

		//��������ص�״̬
		checkGatewayStatus();

		//���ü��ʱ��
		serverLastCheckStaus = timestamp;
	}





	//���serverGatewayLastActiveTime �����ʱ�䳬��30�������
	checkGatewayHeartbeat();

}

int initHashTable(){

	//��ʼ��1��hash���¼����socket
	if(serverSocketList == NULL){
		MAKE_STD_ZVAL(serverSocketList);
		array_init(serverSocketList);
	}

	if(serverAsyncList == NULL){
		MAKE_STD_ZVAL(serverAsyncList);
		array_init(serverAsyncList);
	}

	if(serverSessionList == NULL){
		MAKE_STD_ZVAL(serverSessionList);
		array_init(serverSessionList);
	}

	if(serverGroup == NULL){
		MAKE_STD_ZVAL(serverGroup);
		array_init(serverGroup);
	}

	if(serverLocalSessionData == NULL){
		MAKE_STD_ZVAL(serverLocalSessionData);
		array_init(serverLocalSessionData);
	}

	if(serverUidData == NULL){
		MAKE_STD_ZVAL(serverUidData);
		array_init(serverUidData);
	}

	serverLastCheckStaus = getMicrotime();
}

//��ʼ���ӽ���
//���ý��̱�ʶ
//ÿ���ӽ��̰�epoll
int workProcessInit(int listenfd,int semid,zval *object TSRMLS_DC){

	if(serverSendList == NULL){
		MAKE_STD_ZVAL(serverSendList);
		array_init(serverSendList);
	}

	ini_seti("memory_limit",-1);

	signal(SIGHUP, tcpClienthandleMaster_signal);	//�յ��������˳��ź� �����˳�
	prctl(PR_SET_PDEATHSIG, SIGHUP);		//Ҫ�󸸽����˳�ʱ�����ź�
	prctl(PR_SET_NAME, "CTcpServerWorker");

	//���epoll�¼�
	int epfd = epoll_create(1024);
	serverEpollFd = epfd;
	if(epfd <= 0){
		//����ʧ��  �ӽ��������˳�
		return -1;
	}

	//�󶨶�����
	struct epoll_event  ev;
    ev.events = EPOLLIN; 
    ev.data.fd = listenfd;

	//�����ź� ��ֹ�����˳�
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);

	//�����list�ռ�
	initHashTable();


	//����gateway������
	serverToGatewaySocket = serverConnectToGateway(object TSRMLS_CC);

	
	struct epoll_event revs[128];
    int n = sizeof(revs)/sizeof(revs[0]);
    int timeout = 3000;
    int num = 0;
	int isGotLock = 0;

	//��������
	while(1) {

		//����ź���
		errno = 0;
		struct sembuf trywait;
		trywait.sem_num = 0;
		trywait.sem_op = -1;
		trywait.sem_flg = SEM_UNDO|IPC_NOWAIT;
        int tryNum = semop(semid , &trywait , 1);

		if (tryNum == 0) {
			if (isGotLock == 0) {
				isGotLock = 1;
				//ע���¼�
				epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);
			}
		}else{
			if (isGotLock) {
				//ɾ���¼�
				epoll_ctl(epfd, EPOLL_CTL_DEL, listenfd, NULL);
				isGotLock = 0;
			}
		}


       //��ʼepoll�¼��ȴ�
	   errno = 0;
       num = epoll_wait(epfd,revs,n,500);

	   //��Ϣǰ��������������ϴ��������
	   serverTimerCallback();
	   
	   if(num > 0){
			isGotLock = handlerTCPEvents(epfd,revs,num,listenfd,semid,isGotLock,object TSRMLS_CC);
	   }

		doServerSendMessage(object TSRMLS_CC);

	   if (isGotLock) {
		
		   	struct sembuf tryrelease;
			tryrelease.sem_num = 0;
			tryrelease.sem_op = 1;
			tryrelease.sem_flg = SEM_UNDO|IPC_NOWAIT;
			int releaseStatus = semop(semid , &tryrelease , 1);
			isGotLock = 0;
		}
    }

    close(epfd);
    return 0;

}


PHP_METHOD(CTcpServer,__construct){
	zval	*eventTable;

	ini_seti("memory_limit",-1);

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

	if(serverGroup != NULL){
		zval_ptr_dtor(&serverGroup);
	}

	if(serverSessionId != NULL){
		efree(serverSessionId);
	}

	if(serverAsyncList != NULL){
		zval_ptr_dtor(&serverAsyncList);
	}

	if(serverSocketList != NULL){
		zval_ptr_dtor(&serverSocketList);
	}

	if(serverSessionList != NULL){
		zval_ptr_dtor(&serverSessionList);
	}

	if(serverLocalSessionData != NULL){
		zval_ptr_dtor(&serverLocalSessionData);
	}

	if(serverUidData != NULL){
		zval_ptr_dtor(&serverUidData);
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
		writeLogs("run as a daemon process..\n");
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

	//ʹ��systemV�ź���
	char	*nowFile,
			workDir[180],
			indexFile[250];   
    getcwd(workDir,sizeof(workDir)); 
	getServerParam("PHP_SELF",&nowFile);
	sprintf(indexFile,"%s/%s",workDir,nowFile);
	efree(nowFile);
	key_t semKey = ftok(indexFile,1);
	errno = 0;
	int semid = semget((key_t)semKey,1,IPC_CREAT|0664);
	if(semid < 0){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[FatalException] can not create systemV semget ,errno[%d]",errno);
		close(listenSocket);
		return;
	}
	//��ʼ��
	int initStatus = semctl(semid , 0 , SETVAL , 1);


	//����ȫ�ֱ��� ����״̬���
	serverAccpetLock = semid;
	serverListenSocket = listenSocket;
	serverMainObject = getThis();

	//fork���ӽ���
	int workerNum = getCreateWorkerNums(getThis() TSRMLS_CC);
	createWorkProcess(workerNum,listenSocket,semid,getThis(),1 TSRMLS_CC);
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


PHP_METHOD(CTcpClient,__construct){

	zval *eventTable;

	ini_seti("memory_limit",-1);

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

	if(clientSessionId != NULL){
		efree(clientSessionId);
		clientSessionId = NULL;
	}
}

PHP_METHOD(CTcpClient,setHeartbeatInterval){

	int		timeInterval = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"l",&timeInterval) == FAILURE){
		RETURN_FALSE;
	}

	RETVAL_ZVAL(getThis(),1,0);
	zend_update_property_long(CTcpClientCe,getThis(),ZEND_STRL("heartbeatInterval"),timeInterval TSRMLS_CC);
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
	errno = 0;
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
	writeLogs("server happend exception,waite to reconnect\n");

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

//����sessionID ʱ���+5λ����� md5
int getSessionID(char **sessionId){

	zval *timestamp;
	microtimeTrue(&timestamp);
	srand(time(NULL));
	int randNum = rand();
	char randString[100];
	sprintf(randString,"%.8f%d",Z_DVAL_P(timestamp),randNum);
	md5(randString,sessionId);
	zval_ptr_dtor(&timestamp);
}

int clientGetSDKInfo(char **sdkInfoString TSRMLS_DC){

	zval *sdkInfo;

	//��ǰSDK�汾
	MAKE_STD_ZVAL(sdkInfo);
	array_init(sdkInfo);
	add_assoc_long(sdkInfo,"sdkVer",clientVersion);
	add_assoc_string(sdkInfo,"sdkName","php extension",1);
	add_assoc_long(sdkInfo,"cpuCore",get_nprocs());
	struct sysinfo si;
    if(0 == sysinfo(&si)){
		add_assoc_long(sdkInfo,"memoryAll",si.totalram);
		add_assoc_long(sdkInfo,"memoryFree",si.freeram);
	}else{
		add_assoc_long(sdkInfo,"memoryAll",-1);
		add_assoc_long(sdkInfo,"memoryFree",-1);
	}
	struct utsname osinfo;
	if(0 == uname(&osinfo)){
		add_assoc_string(sdkInfo,"osName",osinfo.sysname,1);
		add_assoc_string(sdkInfo,"osNode",osinfo.nodename,1);
		add_assoc_string(sdkInfo,"osRelease",osinfo.release,1);
		add_assoc_string(sdkInfo,"osVersion",osinfo.version,1);
	}else{
		add_assoc_string(sdkInfo,"osName","",1);
		add_assoc_string(sdkInfo,"osNode","",1);
		add_assoc_string(sdkInfo,"osRelease","",1);
		add_assoc_string(sdkInfo,"osVersion","",1);
	}

	//sessionID
	add_assoc_string(sdkInfo,"sessionId",clientSessionId,1);
	
	//תΪjson
	json_encode(sdkInfo,sdkInfoString);

}


int processClientSystemMessage(int fd,zval *jsonDecoder,char *stringMessage TSRMLS_DC){

	zval **messageEvent;

	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),1,(void**)&messageEvent);


	//����ϵͳ�¼�
	if(strcmp(Z_STRVAL_PP(messageEvent),"connect") == 0){
		
		//�ظ���������ǰ�ͻ�����Ϣ
		char	*sdkInfo,
				*reportMessage;
		clientGetSDKInfo(&sdkInfo TSRMLS_CC);
		createMessage("clientReport",sdkInfo,2,&reportMessage,2 TSRMLS_CC);

		//���뵽���Ͷ���
		zval *sendMessage;
		MAKE_STD_ZVAL(sendMessage);
		array_init(sendMessage);
		add_index_long(sendMessage,0,fd);
		add_index_string(sendMessage,1,reportMessage,0);
		add_next_index_zval(clientSendList,sendMessage);

		efree(sdkInfo);
	}


	//���������ͨ��Ϻ�  �������ӳɹ�
	if(strcmp(Z_STRVAL_PP(messageEvent),"connected") == 0){

		//�����û�̬connect�¼� WzEsImNvbm5lY3QiLCIiXQ==  [1,"connect",""]
		processClientMessage(fd,"WzEsImNvbm5lY3QiLCIiXQ==" TSRMLS_CC);
	}

}

int processClientMessage(int fd,char *thisMessage TSRMLS_DC){

	char	*base64Decoder;
	zval	*jsonDecoder,
			*eventTable,
			*eventDefault,
			**messageEvent,
			**eventCallback,
			**stringMessage,
			**messageType;

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
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),0,(void**)&messageType);
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),1,(void**)&messageEvent);
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),2,(void**)&stringMessage);

	if(IS_LONG == Z_TYPE_PP(messageType) && 2 == Z_LVAL_PP(messageType)){
		processClientSystemMessage(fd,jsonDecoder,thisMessage TSRMLS_CC);
		efree(base64Decoder);
		zval_ptr_dtor(&jsonDecoder);
		return 1;
	}


	//����Ƿ����ָ����event
	eventTable = zend_read_property(CTcpClientCe,clientPHPObject,ZEND_STRL("eventTable"), 0 TSRMLS_CC);

	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(eventTable),Z_STRVAL_PP(messageEvent),strlen(Z_STRVAL_PP(messageEvent))+1,(void**)&eventCallback) && IS_OBJECT == Z_TYPE_PP(eventCallback) ){

		//��PHP��ص�
		zval	*callParams,
				*connectTimeZval;

		MAKE_STD_ZVAL(callParams);
		object_init_ex(callParams,CSocketClientCe);
		zend_update_property_string(CSocketClientCe,callParams,ZEND_STRL("message"),Z_STRVAL_PP(stringMessage) TSRMLS_CC);
		zend_update_property_long(CSocketClientCe,callParams,ZEND_STRL("socketId"),fd TSRMLS_CC);
		zend_update_property_long(CSocketClientCe,callParams,ZEND_STRL("socketType"),2 TSRMLS_CC);

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

						smart_str_0(&serverTempBuffer[fd]);
						smart_str_free(&serverTempBuffer[fd]);

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

//����socket
int resetSocketConnect(){

	writeLogs("[%d] is try to reconnect server,old socket[%d] ...\n",getpid(),clientSocketId);

	//�ر�socket
	close(clientSocketId);
	
	//��������
	int socket = connectServerPort(clientHost,clientPort TSRMLS_CC);
	if(socket < 0){
		clientStatus = 2;
		writeLogs("[%d] is try to reconnect server Fail ...\n",getpid());
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

	writeLogs("[%d] is try to reconnect server Success,new socket[%d] ...\n",getpid(),socket);
	destoryTimer();
	clientStatus = 1;
	return 1;
}


//�ͻ�������
void clientHeartbeat(){
	
	//WzIsImNsaWVudEhlYXJ0YmVhdCIsIiJd  [2,"clientHeartbeat",""]
	char stringMessage[100];
	sprintf(stringMessage,"%s%c","WzIsImNsaWVudEhlYXJ0YmVhdCIsIiJd",'\n');
	
	//��������ʱ����������
	if(clientStatus != 1){
		return;
	}

	int status = -1;
	while(1){
		errno = 0;
		status = write(clientRecProcessWritePipeFd,stringMessage,strlen(stringMessage));

		//�ӽ����ѽ�����������״̬
		if(status < 0 && errno == 32){
			break;
		}

		if(status == strlen(stringMessage)){
			writeLogs("[%d] send heartbeat package send sucess ...\n",getpid());
			break;
		}
		//����ʧ������������
		usleep(500);
	}
}

void timerCallback(){

	int timestamp = getMicrotime();

	//ÿ10�� ���socket״̬
	if(timestamp - clientLastCheckSocket >= 5){

		//���socket״̬
		checkSocketStatus();

		clientLastCheckSocket = timestamp;
	}
	
	//��ȡ�������
	zval *interval = zend_read_property(CTcpClientCe,clientPHPObject,ZEND_STRL("heartbeatInterval"), 0 TSRMLS_CC);
	if(IS_LONG == Z_TYPE_P(interval) && Z_LVAL_P(interval) > 0 && timestamp -  clientLastHeartbeat >= Z_LVAL_P(interval) ){

		//�ͻ�������
		clientHeartbeat();

		clientLastHeartbeat = timestamp;
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

	//��ȡ/tmpĿ¼���Ƿ����
	char *sessionId = NULL;
	/*if(SUCCESS == fileExist("/tmp/CQuickFrameworkPHPExtensionCTcpClient")){
		file_get_contents("/tmp/CQuickFrameworkPHPExtensionCTcpClient",&sessionId);
		if(strlen(sessionId) != 32){
			efree(sessionId);
			sessionId = NULL;
		}
	}*/

	//û�л����ļ�����������
	if(sessionId == NULL){

		getSessionID(&sessionId);
		
		//д�뻺���ļ�
		//file_put_contents("/tmp/CQuickFrameworkPHPExtensionCTcpClient",sessionId);
	}

	clientSessionId = estrdup(sessionId);
	efree(sessionId);

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


//useType 1������ 2�ͻ���
void createMessage(char *event,char *message,int type,char **stringMessage,int useType TSRMLS_DC){

	//������Ϣ
	zval	*messageZval;
	char	*jsonEncoder,
			*base64Encoder;

	MAKE_STD_ZVAL(messageZval);
	array_init(messageZval);

	add_next_index_long(messageZval,type);
	add_next_index_string(messageZval,event,1);
	add_next_index_string(messageZval,message,1);

	if(useType == 2){
		if(clientSessionId == NULL){
			add_next_index_string(messageZval,"",1);
		}else{
			add_next_index_string(messageZval,clientSessionId,1);
		}
	}
	if(useType == 3){
		if(serverSessionId == NULL){
			add_next_index_string(messageZval,"",1);
		}else{
			add_next_index_string(messageZval,serverSessionId,1);
		}
	}
	
	json_encode(messageZval,&jsonEncoder);
	base64Encode(jsonEncoder,&base64Encoder);
	spprintf(stringMessage,0,"%s%c",base64Encoder,'\n');

	//destroy
	zval_ptr_dtor(&messageZval);
	efree(jsonEncoder);
	efree(base64Encoder);
}


PHP_METHOD(CTcpClient,send){

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
	createMessage(event,message,MSG_USER,&stringMessage,2 TSRMLS_CC);

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

PHP_METHOD(CSocket,getClientInfo)
{
	zval *message = zend_read_property(CSocketCe,getThis(),ZEND_STRL("client"), 0 TSRMLS_CC);
	RETVAL_ZVAL(message,1,0);
}

PHP_METHOD(CSocket,read)
{
	zval *message = zend_read_property(CSocketCe,getThis(),ZEND_STRL("message"), 0 TSRMLS_CC);
	RETVAL_ZVAL(message,1,0);
}

PHP_METHOD(CSocket,send)
{
	char	*message,
			*event;
	int		messageLen = 0,
			eventLen = 0;
	zval	*socket,
			*sendList;
	
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ss",&event,&eventLen,&message,&messageLen) == FAILURE){
		throwServerException("[CTcpServerException] call [CTcpServer->send] params error ,need 2 string");
		RETURN_FALSE;
	}

	zval *socketId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("socketId"), 0 TSRMLS_CC);
	zval *socketType = zend_read_property(CSocketCe,getThis(),ZEND_STRL("socketType"), 0 TSRMLS_CC);

	//��¼һ��������Ϣ
	char *stringMessage;
	createMessage(event,message,MSG_USER,&stringMessage,1 TSRMLS_CC);

	zval *sendMessage;
	MAKE_STD_ZVAL(sendMessage);
	array_init(sendMessage);
	add_index_long(sendMessage,0,Z_LVAL_P(socketId));
	add_index_string(sendMessage,1,stringMessage,0);

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
			zval_ptr_dtor(&sendMessage);
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
		zval_ptr_dtor(&sendMessage);
		RETURN_FALSE;
	}

	//���¶�ȡ
	int n =zend_hash_num_elements(Z_ARRVAL_PP(thisSocketList));
	add_next_index_zval(*thisSocketList,sendMessage);
	RETURN_LONG(n+1);
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
	zval	**sessionId;

	zval *client = zend_read_property(CSocketCe,getThis(),ZEND_STRL("client"), 0 TSRMLS_CC);

	if(IS_ARRAY == Z_TYPE_P(client) && SUCCESS == zend_hash_find(Z_ARRVAL_P(client),"sessionId",strlen("sessionId")+1,(void**)&sessionId) && IS_STRING == Z_TYPE_PP(sessionId) ){
		RETURN_STRING(Z_STRVAL_PP(sessionId),1);
	}
	RETURN_FALSE;
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


PHP_METHOD(CTcpServer,getAllConnection)
{

	//�첽�ص�����
	zval	*callback,
			*saveHandler,
			**thisEvent;

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

	//�������ʧ�� ����������Ϣ
	if(serverToGatewayStatus != 1){
		RETURN_FALSE;
	}

	while(1){
		if(zend_hash_num_elements(Z_ARRVAL_P(serverAsyncList)) <= 1024){
			break;
		}
		//�ȴ��¼������������  ��ֹ���Ȼ���
		usleep(200);
	}

	////��˶�����ӻص� �����¼����ֵ
	char *eventId;
	getSessionID(&eventId);
	MAKE_STD_ZVAL(saveHandler);
	ZVAL_ZVAL(saveHandler,callback,1,0);
	add_assoc_zval(serverAsyncList,eventId,saveHandler);


	//�����ط�������ѯ
	char	*gatewayMessage,
			eventCon[100];
	sprintf(eventCon,"{\"eventId\":\"%s\"}",eventId);
	createMessage("getAllConnection",eventCon,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	sendToGateway(gatewayMessage);

	efree(gatewayMessage);
	efree(eventId);

	RETVAL_ZVAL(getThis(),1,0);

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

PHP_METHOD(CTcpClient,close)
{
	clientStatus = 4;

	//clientMainEpollFd clientSocketId
	ClientCloseSocket(clientMainEpollFd,clientSocketId);

	kill(clientRecProcessId,SIGKILL);
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

}

PHP_METHOD(CTcpServer,sendToSessionId)
{}


void CTcpServer_sendToUid(char *toUid,char *event,zval *message,zval *callback,zval *object TSRMLS_DC){

	zval	*saveHandler,
			*sessionId;
	char	*callback_name,
			*sessionIdString = NULL;

	if(callback != NULL){
		if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
			efree(callback_name);
			throwServerException("[CTcpServerException] call [CTcpServer->sendToUid] the sec param is not a callback function");
			return;
		}
		efree(callback_name);
	}

	//�������ʧ�� ����������Ϣ
	if(serverToGatewayStatus != 1){
		return;
	}

	while(1){
		if(zend_hash_num_elements(Z_ARRVAL_P(serverAsyncList)) <= 1024){
			break;
		}
		//�ȴ��¼������������  ��ֹ���Ȼ���
		usleep(200);

		php_printf("wait event callback\n");
	}

	//��˶�����ӻص� �����¼����ֵ
	char *eventId;
	getSessionID(&eventId);
	if(callback != NULL){
		MAKE_STD_ZVAL(saveHandler);
		ZVAL_ZVAL(saveHandler,callback,1,0);
		add_assoc_zval(serverAsyncList,eventId,saveHandler);
	}

	//�����ط�������ѯ
	char	*gatewayMessage,
			*eventCon,
			trueToUid[120];

	zval	*sendMessage;

	sprintf(trueToUid,"u_%s",toUid);
	MAKE_STD_ZVAL(sendMessage);
	array_init(sendMessage);
	add_assoc_string(sendMessage,"eventId",eventId,1);
	add_assoc_string(sendMessage,"toUid",trueToUid,1);
	add_assoc_string(sendMessage,"event",event,1);
	add_assoc_zval(sendMessage,"message",message);
	json_encode(sendMessage,&eventCon);
	createMessage("forwardMessage",eventCon,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	sendToGateway(gatewayMessage);

	efree(gatewayMessage);
	efree(eventId);
	efree(eventCon);
	zval_ptr_dtor(&sendMessage);
}

PHP_METHOD(CTcpServer,sendToUid)
{
	char	*uid,
			*event;
	int		uidLen = 0,
			eventLen = 0;
	zval	*message,
			*callback = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ssz|z",&uid,&uidLen,&event,&eventLen,&message,&callback) == FAILURE){
		RETURN_FALSE;
	}

	//ת����Ϣ��ָ��UID
	CTcpServer_sendToUid(uid,event,message,callback,getThis() TSRMLS_CC);

}

PHP_METHOD(CSocket,bindUid)
{
	zval	*callback = NULL,
			*saveHandler,
			*sessionId;
	char	*uid = NULL,
			*callback_name,
			*sessionIdString = NULL;
	int		uidLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|z",&uid,&uidLen,&callback) == FAILURE){
		RETURN_FALSE;
	}

	if(callback != NULL){
		if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
			efree(callback_name);
			throwServerException("[CTcpServerException] call [CTcpServer->bindUid] the sec param is not a callback function");
			RETVAL_ZVAL(getThis(),1,0);
			return;
		}
		efree(callback_name);
	}

	//�������ʧ�� ����������Ϣ
	if(serverToGatewayStatus != 1){
		RETURN_FALSE;
	}

	//���client session
	sessionId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("client"), 0 TSRMLS_CC);
	zval **sessionIdZval;
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(sessionId),"sessionId",strlen("sessionId")+1,(void**)&sessionIdZval)){
		sessionIdString = Z_STRVAL_PP(sessionIdZval);
	}else{
		RETURN_FALSE;
	}

	while(1){
		if(zend_hash_num_elements(Z_ARRVAL_P(serverAsyncList)) <= 1024){
			break;
		}
		//�ȴ��¼������������  ��ֹ���Ȼ���
		usleep(200);
	}

	//��˶�����ӻص� �����¼����ֵ
	char *eventId;
	getSessionID(&eventId);
	if(callback != NULL){
		MAKE_STD_ZVAL(saveHandler);
		ZVAL_ZVAL(saveHandler,callback,1,0);
		add_assoc_zval(serverAsyncList,eventId,saveHandler);
	}

	//���ؼ�¼bindUid��¼
	char trueUid[120];
	sprintf(trueUid,"u_%s",uid);
	add_assoc_string(serverUidData,trueUid,sessionIdString,1);

	//�����ط�������ѯ
	char	*gatewayMessage,
			eventCon[100];
	sprintf(eventCon,"{\"eventId\":\"%s\",\"uid\":\"%s\",\"clientSessionId\":\"%s\"}",eventId,trueUid,sessionIdString);
	createMessage("bindUid",eventCon,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	sendToGateway(gatewayMessage);

	efree(gatewayMessage);
	efree(eventId);

	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CSocket,unBindUid)
{
	zval	*callback = NULL,
			*saveHandler,
			*sessionId;
	char	*uid = NULL,
			*callback_name,
			*sessionIdString = NULL;
	int		uidLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|z",&uid,&uidLen,&callback) == FAILURE){
		RETURN_FALSE;
	}

	if(callback != NULL){
		if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
			efree(callback_name);
			throwServerException("[CTcpServerException] call [CTcpServer->bindUid] the sec param is not a callback function");
			RETVAL_ZVAL(getThis(),1,0);
			return;
		}
		efree(callback_name);
	}

	//�������ʧ�� ����������Ϣ
	if(serverToGatewayStatus != 1){
		RETURN_FALSE;
	}

	//���client session
	sessionId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("client"), 0 TSRMLS_CC);
	zval **sessionIdZval;
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(sessionId),"sessionId",strlen("sessionId")+1,(void**)&sessionIdZval)){
		sessionIdString = Z_STRVAL_PP(sessionIdZval);
	}else{
		RETURN_FALSE;
	}

	while(1){
		if(zend_hash_num_elements(Z_ARRVAL_P(serverAsyncList)) <= 1024){
			break;
		}
		//�ȴ��¼������������  ��ֹ���Ȼ���
		usleep(200);
	}

	//��˶�����ӻص� �����¼����ֵ
	char *eventId;
	getSessionID(&eventId);
	if(callback != NULL){
		MAKE_STD_ZVAL(saveHandler);
		ZVAL_ZVAL(saveHandler,callback,1,0);
		add_assoc_zval(serverAsyncList,eventId,saveHandler);
	}


	//���ؼ�¼bindUid��¼
	char trueUid[120];
	sprintf(trueUid,"u_%s",uid);
	zend_hash_del(Z_ARRVAL_P(serverUidData),trueUid,strlen(uid)+1);

	//�����ط�������ѯ
	char	*gatewayMessage,
			eventCon[100];
	sprintf(eventCon,"{\"eventId\":\"%s\",\"uid\":\"%s\",\"clientSessionId\":\"%s\"}",eventId,trueUid,sessionIdString);
	createMessage("unBindUid",eventCon,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	sendToGateway(gatewayMessage);

	efree(gatewayMessage);
	efree(eventId);

	RETVAL_ZVAL(getThis(),1,0);
}


PHP_METHOD(CSocket,joinGroup)
{
	zval	*callback = NULL,
			*saveHandler,
			*sessionId;
	char	*groupName = NULL,
			*callback_name,
			*sessionIdString = NULL;
	int		groupNameLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|z",&groupName,&groupNameLen,&callback) == FAILURE){
		RETURN_FALSE;
	}

	if(callback != NULL){
		if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
			efree(callback_name);
			throwServerException("[CTcpServerException] call [CTcpServer->joinGroup] the sec param is not a callback function");
			RETVAL_ZVAL(getThis(),1,0);
			return;
		}
		efree(callback_name);
	}

	//�������ʧ�� ����������Ϣ
	if(serverToGatewayStatus != 1){
		RETURN_FALSE;
	}

	//���client session
	sessionId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("client"), 0 TSRMLS_CC);
	zval **sessionIdZval;
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(sessionId),"sessionId",strlen("sessionId")+1,(void**)&sessionIdZval)){
		sessionIdString = Z_STRVAL_PP(sessionIdZval);
	}else{
		RETURN_FALSE;
	}

	while(1){
		if(zend_hash_num_elements(Z_ARRVAL_P(serverAsyncList)) <= 1024){
			break;
		}
		//�ȴ��¼������������  ��ֹ���Ȼ���
		usleep(200);
	}

	//��˶�����ӻص� �����¼����ֵ
	char *eventId;
	getSessionID(&eventId);
	if(callback != NULL){
		MAKE_STD_ZVAL(saveHandler);
		ZVAL_ZVAL(saveHandler,callback,1,0);
		add_assoc_zval(serverAsyncList,eventId,saveHandler);
	}

	add_assoc_string(serverGroup,Z_STRVAL_PP(sessionIdZval),groupName,1);

	//�����ط�������ѯ
	char	*gatewayMessage,
			eventCon[100];
	sprintf(eventCon,"{\"eventId\":\"%s\",\"group\":\"%s\",\"clientSessionId\":\"%s\"}",eventId,groupName,sessionIdString);
	createMessage("joinGroup",eventCon,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	sendToGateway(gatewayMessage);

	efree(gatewayMessage);
	efree(eventId);

	RETVAL_ZVAL(getThis(),1,0);

}

PHP_METHOD(CSocket,getGroup)
{
	zval	*callback,
			*saveHandler,
			*sessionId;
	char	*groupName = NULL,
			*callback_name,
			*sessionIdString = NULL;
	int		groupNameLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sz",&groupName,&groupNameLen,&callback) == FAILURE){
		RETURN_FALSE;
	}

	if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
        efree(callback_name);
		throwServerException("[CTcpServerException] call [CTcpServer->getGroup] the sec param is not a callback function");
        RETVAL_ZVAL(getThis(),1,0);
        return;
    }
	efree(callback_name);

	//�������ʧ�� ����������Ϣ
	if(serverToGatewayStatus != 1){
		RETURN_FALSE;
	}

	while(1){
		if(zend_hash_num_elements(Z_ARRVAL_P(serverAsyncList)) <= 1024){
			break;
		}
		//�ȴ��¼������������  ��ֹ���Ȼ���
		usleep(200);
	}

	//��˶�����ӻص� �����¼����ֵ
	char *eventId;
	getSessionID(&eventId);
	MAKE_STD_ZVAL(saveHandler);
	ZVAL_ZVAL(saveHandler,callback,1,0);
	add_assoc_zval(serverAsyncList,eventId,saveHandler);


	//�����ط�������ѯ
	char	*gatewayMessage,
			eventCon[100];
	sprintf(eventCon,"{\"eventId\":\"%s\",\"group\":\"%s\"}",eventId,groupName);
	createMessage("getGroup",eventCon,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	sendToGateway(gatewayMessage);

	efree(gatewayMessage);
	efree(eventId);

	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CTcpServer,getGroup)
{
	zval	*callback,
			*saveHandler,
			*sessionId;
	char	*groupName = NULL,
			*callback_name,
			*sessionIdString = NULL;
	int		groupNameLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sz",&groupName,&groupNameLen,&callback) == FAILURE){
		RETURN_FALSE;
	}

	if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
        efree(callback_name);
		throwServerException("[CTcpServerException] call [CTcpServer->getGroup] the sec param is not a callback function");
        RETVAL_ZVAL(getThis(),1,0);
        return;
    }
	efree(callback_name);

	//�������ʧ�� ����������Ϣ
	if(serverToGatewayStatus != 1){
		RETURN_FALSE;
	}

	while(1){
		if(zend_hash_num_elements(Z_ARRVAL_P(serverAsyncList)) <= 1024){
			break;
		}
		//�ȴ��¼������������  ��ֹ���Ȼ���
		usleep(200);
	}

	//��˶�����ӻص� �����¼����ֵ
	char *eventId;
	getSessionID(&eventId);
	MAKE_STD_ZVAL(saveHandler);
	ZVAL_ZVAL(saveHandler,callback,1,0);
	add_assoc_zval(serverAsyncList,eventId,saveHandler);

	//�����ط�������ѯ
	char	*gatewayMessage,
			eventCon[100];
	sprintf(eventCon,"{\"eventId\":\"%s\",\"group\":\"%s\"}",eventId,groupName);
	createMessage("getGroup",eventCon,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	sendToGateway(gatewayMessage);

	efree(gatewayMessage);
	efree(eventId);

	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CSocket,leaveGroup)
{
	zval	*callback = NULL,
			*saveHandler,
			*sessionId;
	char	*groupName = NULL,
			*callback_name,
			*sessionIdString = NULL;
	int		groupNameLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|z",&groupName,&groupNameLen,&callback) == FAILURE){
		RETURN_FALSE;
	}

	if(callback != NULL){
		if (!zend_is_callable(callback, 0, &callback_name TSRMLS_CC)) {
			efree(callback_name);
			throwServerException("[CTcpServerException] call [CTcpServer->joinGroup] the sec param is not a callback function");
			RETVAL_ZVAL(getThis(),1,0);
			return;
		}
		efree(callback_name);
	}

	//�������ʧ�� ����������Ϣ
	if(serverToGatewayStatus != 1){
		RETURN_FALSE;
	}

	//���client session
	sessionId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("client"), 0 TSRMLS_CC);
	zval **sessionIdZval;
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(sessionId),"sessionId",strlen("sessionId")+1,(void**)&sessionIdZval)){
		sessionIdString = Z_STRVAL_PP(sessionIdZval);
	}else{
		RETURN_FALSE;
	}

	while(1){
		if(zend_hash_num_elements(Z_ARRVAL_P(serverAsyncList)) <= 1024){
			break;
		}
		//�ȴ��¼������������  ��ֹ���Ȼ���
		usleep(200);
	}

	//��˶�����ӻص� �����¼����ֵ
	char *eventId;
	getSessionID(&eventId);

	if(callback != NULL){
		MAKE_STD_ZVAL(saveHandler);
		ZVAL_ZVAL(saveHandler,callback,1,0);
		add_assoc_zval(serverAsyncList,eventId,saveHandler);
	}


	zend_hash_del(Z_ARRVAL_P(serverGroup),Z_STRVAL_PP(sessionIdZval),strlen(Z_STRVAL_PP(sessionIdZval))+1);


	//�����ط�������ѯ
	char	*gatewayMessage,
			eventCon[100];
	sprintf(eventCon,"{\"eventId\":\"%s\",\"group\":\"%s\",\"clientSessionId\":\"%s\"}",eventId,groupName,sessionIdString);
	createMessage("leaveGroup",eventCon,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	sendToGateway(gatewayMessage);

	efree(gatewayMessage);
	efree(eventId);

	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CTcpServer,broadcastToGroup)
{
}

PHP_METHOD(CTcpServer,broadcast)
{

	char	*message,
			*event;
	int		messageLen = 0,
			eventLen = 0;
	zval	*socket,
			*sendList;
	
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ss",&event,&eventLen,&message,&messageLen) == FAILURE){
		throwServerException("[CTcpServerException] call [CTcpServer->send] params error ,need 2 string");
		RETURN_FALSE;
	}

	//�������ʧ�� ����������Ϣ
	if(serverToGatewayStatus != 1){
		return;
	}

	//��¼һ��������Ϣ
	zval	*messageJson;
	char	*jsonString;
	MAKE_STD_ZVAL(messageJson);
	array_init(messageJson);
	add_index_long(messageJson,0,MSG_USER);
	add_index_string(messageJson,1,event,1);
	add_index_string(messageJson,2,message,1);
	json_encode(messageJson,&jsonString);

	char *gatewayMessage;
	createMessage("broadcast",jsonString,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	sendToGateway(gatewayMessage);

	zval_ptr_dtor(&messageJson);
	efree(jsonString);
	efree(gatewayMessage);
}

PHP_METHOD(CSocket,setSession)
{
	zval	*callback = NULL,
			*saveHandler,
			*sessionId,
			*keyData = NULL,
			**thisSessionData;

	char	*keyName = NULL,
			*callback_name,
			*sessionIdString = NULL;
	int		keyNameLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sz",&keyName,&keyNameLen,&keyData) == FAILURE){
		RETURN_FALSE;
	}

	//���client session
	sessionId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("client"), 0 TSRMLS_CC);
	zval **sessionIdZval;
	if(IS_ARRAY == Z_TYPE_P(sessionId) && SUCCESS == zend_hash_find(Z_ARRVAL_P(sessionId),"sessionId",strlen("sessionId")+1,(void**)&sessionIdZval)){
		sessionIdString = Z_STRVAL_PP(sessionIdZval);
	}else{
		RETURN_FALSE;
	}

	//���ؼ�¼session serverLocalSessionData
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(serverLocalSessionData),sessionIdString,strlen(sessionIdString)+1,(void**)&thisSessionData) && IS_ARRAY == Z_TYPE_PP(thisSessionData) ){
	}else{
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		add_assoc_zval(serverLocalSessionData,sessionIdString,saveArray);
		zend_hash_find(Z_ARRVAL_P(serverLocalSessionData),sessionIdString,strlen(sessionIdString)+1,(void**)&thisSessionData);
	}

	//����Ϣ����������sessionData��
	zval *saveLocal;
	MAKE_STD_ZVAL(saveLocal);
	ZVAL_ZVAL(saveLocal,keyData,1,0);
	add_assoc_zval(*thisSessionData,keyName,saveLocal);

	RETURN_TRUE;
}

PHP_METHOD(CSocket,getSession)
{
	zval	*callback = NULL,
			*saveHandler,
			*sessionId,
			*keyData = NULL,
			**thisSessionData,
			**thisReturnData;

	char	*keyName = NULL,
			*callback_name,
			*sessionIdString = NULL;
	int		keyNameLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&keyName,&keyNameLen) == FAILURE){
		RETURN_FALSE;
	}

	//���client session
	sessionId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("client"), 0 TSRMLS_CC);
	zval **sessionIdZval;
	if(IS_ARRAY == Z_TYPE_P(sessionId) && SUCCESS == zend_hash_find(Z_ARRVAL_P(sessionId),"sessionId",strlen("sessionId")+1,(void**)&sessionIdZval)){
		sessionIdString = Z_STRVAL_PP(sessionIdZval);
	}else{
		RETURN_FALSE;
	}

	//���ؼ�¼session serverLocalSessionData
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(serverLocalSessionData),sessionIdString,strlen(sessionIdString)+1,(void**)&thisSessionData) && IS_ARRAY == Z_TYPE_PP(thisSessionData) ){

		if(0 == zend_hash_num_elements(Z_ARRVAL_PP(thisSessionData)) ){
			zend_hash_del(Z_ARRVAL_P(serverLocalSessionData),sessionIdString,strlen(sessionIdString)+1);
			RETURN_NULL();
		}

		if(SUCCESS == zend_hash_find(Z_ARRVAL_PP(thisSessionData),keyName,strlen(keyName)+1,(void**)&thisReturnData)){
			RETURN_ZVAL(*thisReturnData,1,0);
		}
	}

	RETURN_NULL();
}

PHP_METHOD(CSocket,delSession)
{
	zval	*callback = NULL,
			*saveHandler,
			*sessionId,
			*keyData = NULL,
			**thisSessionData,
			**thisReturnData;

	char	*keyName = NULL,
			*callback_name,
			*sessionIdString = NULL;
	int		keyNameLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&keyName,&keyNameLen) == FAILURE){
		RETURN_FALSE;
	}

	//���client session
	sessionId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("client"), 0 TSRMLS_CC);
	zval **sessionIdZval;
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(sessionId),"sessionId",strlen("sessionId")+1,(void**)&sessionIdZval)){
		sessionIdString = Z_STRVAL_PP(sessionIdZval);
	}else{
		RETURN_FALSE;
	}

	//���ؼ�¼session serverLocalSessionData
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(serverLocalSessionData),sessionIdString,strlen(sessionIdString)+1,(void**)&thisSessionData) && IS_ARRAY == Z_TYPE_PP(thisSessionData) ){

		if(0 == zend_hash_num_elements(Z_ARRVAL_PP(thisSessionData)) ){
			zend_hash_del(Z_ARRVAL_P(serverLocalSessionData),sessionIdString,strlen(sessionIdString)+1);
			RETURN_FALSE;
		}

		if(zend_hash_exists(Z_ARRVAL_PP(thisSessionData),keyName,strlen(keyName)+1)){
			zend_hash_del(Z_ARRVAL_PP(thisSessionData),keyName,strlen(keyName)+1);
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}

PHP_METHOD(CSocket,clearSession)
{
	zval	*callback = NULL,
			*saveHandler,
			*sessionId,
			*keyData = NULL,
			**thisSessionData,
			**thisReturnData;

	char	*keyName = NULL,
			*callback_name,
			*sessionIdString = NULL;
	int		keyNameLen = 0;


	//���client session
	sessionId = zend_read_property(CSocketCe,getThis(),ZEND_STRL("client"), 0 TSRMLS_CC);
	zval **sessionIdZval;
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(sessionId),"sessionId",strlen("sessionId")+1,(void**)&sessionIdZval)){
		sessionIdString = Z_STRVAL_PP(sessionIdZval);
	}else{
		RETURN_FALSE;
	}

	//���ؼ�¼session serverLocalSessionData
	if(zend_hash_exists(Z_ARRVAL_P(serverLocalSessionData),sessionIdString,strlen(sessionIdString)+1) ){
		zend_hash_del(Z_ARRVAL_P(serverLocalSessionData),sessionIdString,strlen(sessionIdString)+1);
		RETURN_TRUE;
	}

	RETURN_FALSE;
}

PHP_METHOD(CSocketClient,read)
{
	zval *message = zend_read_property(CSocketClientCe,getThis(),ZEND_STRL("message"), 0 TSRMLS_CC);
	RETVAL_ZVAL(message,1,0);
}

PHP_METHOD(CSocketClient,send)
{
	char	*message,
			*event;
	int		messageLen = 0,
			eventLen = 0;
	zval	*socket,
			*sendList;
	
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ss",&event,&eventLen,&message,&messageLen) == FAILURE){
		throwServerException("[CTcpClientException] call [CSocketClient->send] params error ,need 2 string");
		RETURN_FALSE;
	}


	//��¼һ��������Ϣ
	char *stringMessage;
	createMessage(event,message,MSG_USER,&stringMessage,2 TSRMLS_CC);

	zval *socketId = zend_read_property(CSocketClientCe,getThis(),ZEND_STRL("socketId"), 0 TSRMLS_CC);
	zval *socketType = zend_read_property(CSocketClientCe,getThis(),ZEND_STRL("socketType"), 0 TSRMLS_CC);

	zval *sendMessage;
	MAKE_STD_ZVAL(sendMessage);
	array_init(sendMessage);
	add_index_long(sendMessage,0,Z_LVAL_P(socketId));
	add_index_string(sendMessage,1,stringMessage,0);

	//ͬ����Ϣ����
	int len = zend_hash_num_elements(Z_ARRVAL_P(clientSendList));
	if(len >= MAX_SENDLIST_LEN){
		zval_ptr_dtor(&sendMessage);
		RETURN_FALSE;
	}
	add_next_index_zval(clientSendList,sendMessage);
	RETVAL_LONG(len+1);
}

PHP_METHOD(CTcpGateway,__construct)
{}

PHP_METHOD(CTcpGateway,__destruct)
{
	if(gatewaySocketList != NULL){
		zval_ptr_dtor(&gatewaySocketList);
	}
	if(gatewaySessionList != NULL){
		zval_ptr_dtor(&gatewaySessionList);
	}
	if(gatewayUserSessionList != NULL){
		zval_ptr_dtor(&gatewayUserSessionList);
	}
	if(gatewaySendList != NULL){
		zval_ptr_dtor(&gatewaySendList);
	}
	if(gatewayGroupList != NULL){
		zval_ptr_dtor(&gatewayGroupList);
	}
	if(gatewayUidSessionList != NULL){
		zval_ptr_dtor(&gatewayUidSessionList);
	}
	if(gatewaySessionData != NULL){
		zval_ptr_dtor(&gatewaySessionData);
	}
}

PHP_METHOD(CTcpGateway,getInstance)
{
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


	selfInstace = zend_read_static_property(CTcpGatewayCe,ZEND_STRL("instance"),1 TSRMLS_CC);

	//���ΪNULL�����ΪZvalHashtable
	if(IS_ARRAY != Z_TYPE_P(selfInstace)){
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		zend_update_static_property(CTcpGatewayCe,ZEND_STRL("instance"),saveArray TSRMLS_CC);
		zval_ptr_dtor(&saveArray);
		selfInstace = zend_read_static_property(CTcpGatewayCe,ZEND_STRL("instance"),1 TSRMLS_CC);
	}

	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(selfInstace),key,strlen(key)+1,(void**)&instaceSaveZval) ){
		RETVAL_ZVAL(*instaceSaveZval,1,0);
	}else{

		zval	*object;

		MAKE_STD_ZVAL(object);
		object_init_ex(object,CTcpGatewayCe);

		//ִ���乹���� ���������
		if (CTcpGatewayCe->constructor) {
			zval	constructVal,
					constructReturn;
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal, CTcpGatewayCe->constructor->common.function_name, 0);
			call_user_function(NULL, &object, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
			zval_dtor(&constructReturn);
		}

		//������������ֵ����instance��̬����
		add_assoc_zval(selfInstace,key,object);
		zend_update_static_property(CTcpGatewayCe,ZEND_STRL("instance"),selfInstace TSRMLS_CC);

		RETURN_ZVAL(object,1,0);
	}
}

PHP_METHOD(CTcpGateway,bind)
{
	char	*host;
	int		hostLen = 0;
	long	port = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sl",&host,&hostLen,&port) == FAILURE){
		RETURN_FALSE;
	}

	if(port == 0){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[CTCPGatewayException] call [CTCPGateway->bind] the port must available port");
		return;
	}

	zend_update_property_string(CTcpGatewayCe,getThis(),ZEND_STRL("host"),host TSRMLS_CC);
	zend_update_property_long(CTcpGatewayCe,getThis(),ZEND_STRL("port"),port TSRMLS_CC);

	RETVAL_ZVAL(getThis(),1,0);
}

PHP_METHOD(CTcpServer,gateway){

	char	*host;
	int		hostLen = 0;
	long	port = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sl",&host,&hostLen,&port) == FAILURE){
		RETURN_FALSE;
	}

	if(port == 0){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[CTCPServerException] call [CTCPServer->gateway] the port must available port");
		return;
	}

	zend_update_property_string(CTcpServerCe,getThis(),ZEND_STRL("gatewayHost"),host TSRMLS_CC);
	zend_update_property_long(CTcpServerCe,getThis(),ZEND_STRL("gatewayPort"),port TSRMLS_CC);
	zend_update_property_long(CTcpServerCe,getThis(),ZEND_STRL("gatewayUse"),1 TSRMLS_CC);

	RETVAL_ZVAL(getThis(),1,0);
}

//�������Ͷ��� ���worker������Ϣ
void gatewayCloseWokerSocket(int fd);
void doGatewaySendMessageList(){

	int		i,h,j,k,writeStatus;
	zval	**thisSocketList,
			**thisMessage;
	char	*socketIdString,
			*messageIdString;
	ulong	socketId,
			messageId;

	h = zend_hash_num_elements(Z_ARRVAL_P(gatewaySendList));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(gatewaySendList));
	for(i = 0 ; i < h ; i++){
		zend_hash_get_current_key(Z_ARRVAL_P(gatewaySendList),&socketIdString,&socketId,0);
		zend_hash_get_current_data(Z_ARRVAL_P(gatewaySendList),(void**)&thisSocketList);

		//Ϊ�����Թ�����Ϣ
		if(IS_ARRAY != Z_TYPE_PP(thisSocketList)){
			zend_hash_move_forward(Z_ARRVAL_P(gatewaySendList));
			continue;
		}

		k = zend_hash_num_elements(Z_ARRVAL_PP(thisSocketList));
		zend_hash_internal_pointer_reset(Z_ARRVAL_PP(thisSocketList));
		for(j = 0 ; j < k ; j++){
			zend_hash_get_current_data(Z_ARRVAL_PP(thisSocketList),(void**)&thisMessage);
			zend_hash_get_current_key(Z_ARRVAL_PP(thisSocketList),&messageIdString,&messageId,0);

			//ת����socketId
			errno = 0;
			writeStatus = write(socketId,Z_STRVAL_PP(thisMessage),Z_STRLEN_PP(thisMessage));

			//д��ʧ��
			if(writeStatus <= 0){	
				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
					break;
				}else{
				
					//worker�����쳣 
					gatewayCloseWokerSocket(socketId);
					break;
				}
			}

			//д��һ��
			if(writeStatus < Z_STRLEN_PP(thisMessage)){
				char *notSendMessage;
				substr(Z_STRVAL_PP(thisMessage),writeStatus,Z_STRLEN_PP(thisMessage)-writeStatus,&notSendMessage);
				add_index_string(*thisMessage,1,notSendMessage,0);
				break;
			}



			//ɾ������Ϣ
			zend_hash_move_forward(Z_ARRVAL_PP(thisSocketList));
			zend_hash_index_del(Z_ARRVAL_PP(thisSocketList),messageId);
		}

		zend_hash_move_forward(Z_ARRVAL_P(gatewaySendList));
	}
}


//ת����Ϣ�����е�worker
void gatewayBroadcastMessage(char *thisMessage,zval *object TSRMLS_CC){

	int		i,h;
	zval	**thisSocketSendList;
	char	*key;
	ulong	uKey;

	h = zend_hash_num_elements(Z_ARRVAL_P(gatewaySocketList));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(gatewaySocketList));

	for(i = 0 ; i < h;i++){
		zend_hash_get_current_key(Z_ARRVAL_P(gatewaySocketList),&key,&uKey,0);

		//���sendList
		if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(gatewaySendList),(int)uKey,(void**)&thisSocketSendList) && IS_ARRAY == Z_TYPE_PP(thisSocketSendList) ){
		}else{
			zval *saveArray;
			MAKE_STD_ZVAL(saveArray);
			array_init(saveArray);
			add_index_zval(gatewaySendList,(int)uKey,saveArray);
			zend_hash_index_find(Z_ARRVAL_P(gatewaySendList),(int)uKey,(void**)&thisSocketSendList);
		}

		//��ǰ����Ϣ
		char *sendMessage;
		spprintf(&sendMessage,0,"%s%c",thisMessage,'\n');
		add_next_index_string(*thisSocketSendList,sendMessage,0);

		zend_hash_move_forward(Z_ARRVAL_P(gatewaySocketList));
	}
}

void gatewaySendMessageToSocket(int fd,char *thisMessage,zval *object TSRMLS_DC){

	zval	**thisSocketSendList;

	if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(gatewaySendList),fd,(void**)&thisSocketSendList) && IS_ARRAY == Z_TYPE_PP(thisSocketSendList) ){
	}else{
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		add_index_zval(gatewaySendList,fd,saveArray);
		zend_hash_index_find(Z_ARRVAL_P(gatewaySendList),fd,(void**)&thisSocketSendList);
	}

	add_next_index_string(*thisSocketSendList,thisMessage,1);
}

//��Ӧ���е�sessionList
void gatewayResponseAllSession(int fd,char *message,zval *object TSRMLS_DC){

	//��ȡ�¼�id
	zval	*eventJson,
			**eventId;
	json_decode(message,&eventJson);
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"eventId",8,(void**)&eventId) && IS_STRING == Z_TYPE_PP(eventId)){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}


	zval	*allSessionList;
	char	*key;
	ulong	uKey;

	MAKE_STD_ZVAL(allSessionList);
	array_init(allSessionList);

	int		i,h;
	h = zend_hash_num_elements(Z_ARRVAL_P(gatewayUserSessionList));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(gatewayUserSessionList));
	for(i = 0 ; i < h;i++){
		zend_hash_get_current_key(Z_ARRVAL_P(gatewayUserSessionList),&key,&uKey,0);
		add_next_index_string(allSessionList,key,1);
		zend_hash_move_forward(Z_ARRVAL_P(gatewayUserSessionList));
	}

	//��������
	zval	*responseData;
	MAKE_STD_ZVAL(responseData);
	array_init(responseData);
	add_assoc_string(responseData,"eventId",Z_STRVAL_PP(eventId),1);
	add_assoc_zval(responseData,"data",allSessionList);


	//תjson
	char	*jsonString,
			*gatewayMessage;

	json_encode(responseData,&jsonString);
	createMessage("getAllConnection",jsonString,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	gatewaySendMessageToSocket(fd,gatewayMessage,object TSRMLS_CC);

	//destory
	zval_ptr_dtor(&eventJson);
	efree(gatewayMessage);
	zval_ptr_dtor(&allSessionList);
	zval_ptr_dtor(&responseData);
}

//type 1����Ⱥ��  2�˳�Ⱥ��
void gatewayEditGroup(int fd,char *message,int type,zval *object TSRMLS_DC){

	//��ȡ�¼�id
	zval	*eventJson,
			**group,
			**sessionId,
			**thisGroup,
			**eventId;

	json_decode(message,&eventJson);

	//eventId
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"eventId",8,(void**)&eventId) && IS_STRING == Z_TYPE_PP(eventId)){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}

	if(	SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"group",6,(void**)&group) && IS_STRING == Z_TYPE_PP(group) && 
		SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"clientSessionId",strlen("clientSessionId")+1,(void**)&sessionId) && IS_STRING == Z_TYPE_PP(sessionId)	){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}

	//�жϵ�ǰȺ���Ƿ����
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(gatewayGroupList),Z_STRVAL_PP(group),strlen(Z_STRVAL_PP(group))+1,(void**)&thisGroup) && IS_ARRAY == Z_TYPE_PP(thisGroup)){
	}else{
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		add_assoc_zval(gatewayGroupList,Z_STRVAL_PP(group),saveArray);
		zend_hash_find(Z_ARRVAL_P(gatewayGroupList),Z_STRVAL_PP(group),strlen(Z_STRVAL_PP(group))+1,(void**)&thisGroup);
	}
	
	//��Ⱥ������ӿͻ���sessionId
	if(type == 1){
		add_assoc_long(*thisGroup,Z_STRVAL_PP(sessionId),0);
		writeLogs("[CTcpGateway] worker call joinGroup now gourp[%s] list is len:[%d] \n",Z_STRVAL_PP(group),zend_hash_num_elements(Z_ARRVAL_PP(thisGroup)));
	}
	
	//ɾ��Ⱥ��
	if(type == 2){
		zend_hash_del(Z_ARRVAL_PP(thisGroup),Z_STRVAL_PP(sessionId),strlen(Z_STRVAL_PP(sessionId))+1);
		writeLogs("[CTcpGateway] worker call leaveGroup now gourp[%s] list is len:[%d] \n",Z_STRVAL_PP(group),zend_hash_num_elements(Z_ARRVAL_PP(thisGroup)));
	}

	//�����ص�
	char	callbackInfo[200],
			*gatewayMessage;
	int	nums = zend_hash_num_elements(Z_ARRVAL_PP(thisGroup));
	sprintf(callbackInfo,"{\"eventId\":\"%s\",\"data\":{\"result\":true,\"members\":%d,\"sessionId\":\"%s\"}}",Z_STRVAL_PP(eventId),nums,Z_STRVAL_PP(sessionId));
	createMessage("joinGroup",callbackInfo,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	gatewaySendMessageToSocket(fd,gatewayMessage,object TSRMLS_CC);
	efree(gatewayMessage);

	//destory
	zval_ptr_dtor(&eventJson);
}

void gatewayGetGroup(int fd,char *message,zval *object TSRMLS_DC){

	//��ȡ�¼�id
	zval	*eventJson,
			**group,
			**sessionId,
			**thisGroup,
			**eventId;

	json_decode(message,&eventJson);

	//eventId
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"eventId",8,(void**)&eventId) && IS_STRING == Z_TYPE_PP(eventId)){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}

	if(	SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"group",6,(void**)&group) && IS_STRING == Z_TYPE_PP(group)	){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}


	//�жϵ�ǰȺ���Ƿ����
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(gatewayGroupList),Z_STRVAL_PP(group),strlen(Z_STRVAL_PP(group))+1,(void**)&thisGroup) && IS_ARRAY == Z_TYPE_PP(thisGroup)){
	}else{
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		add_assoc_zval(gatewayGroupList,Z_STRVAL_PP(group),saveArray);
		zend_hash_find(Z_ARRVAL_P(gatewayGroupList),Z_STRVAL_PP(group),strlen(Z_STRVAL_PP(group))+1,(void**)&thisGroup);
	}

	zval	*returnMessage;
	MAKE_STD_ZVAL(returnMessage);
	array_init(returnMessage);

	int		i,h;
	char	*key;
	ulong	uKey;
	h = zend_hash_num_elements(Z_ARRVAL_PP(thisGroup));
	zend_hash_internal_pointer_reset(Z_ARRVAL_PP(thisGroup));
	for(i = 0 ; i < h ;i++){
		zend_hash_get_current_key(Z_ARRVAL_PP(thisGroup),&key,&uKey,0);
		add_next_index_string(returnMessage,key,1);
		zend_hash_move_forward(Z_ARRVAL_PP(thisGroup));
	}

	//תΪjson
	zval	*callbackMessage;
	char	*callbackJson;
	MAKE_STD_ZVAL(callbackMessage);
	array_init(callbackMessage);
	add_assoc_string(callbackMessage,"eventId",Z_STRVAL_PP(eventId),1);
	add_assoc_zval(callbackMessage,"data",returnMessage);
	json_encode(callbackMessage,&callbackJson);


	//��Ӧ��Ϣ
	char	*gatewayMessage;
	createMessage("joinGroup",callbackJson,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	gatewaySendMessageToSocket(fd,gatewayMessage,object TSRMLS_CC);
	efree(gatewayMessage);

	//destory
	efree(callbackJson);
	zval_ptr_dtor(&eventJson);
	zval_ptr_dtor(&returnMessage);
	zval_ptr_dtor(&callbackMessage);
}

//����UID
void gatewayEditUid(int fd,char *message,int type,zval *object TSRMLS_DC)
{
	//��ȡ�¼�id
	zval	*eventJson,
			**uid,
			**sessionId,
			**thisGroup,
			**eventId;

	json_decode(message,&eventJson);

	//eventId
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"eventId",8,(void**)&eventId) && IS_STRING == Z_TYPE_PP(eventId)){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}

	if(	SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"uid",strlen("uid")+1,(void**)&uid) && 
		SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"clientSessionId",strlen("clientSessionId")+1,(void**)&sessionId) && IS_STRING == Z_TYPE_PP(sessionId) ){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}

	char trueUid[120];
	if(IS_STRING == Z_TYPE_PP(uid)){
		sprintf(trueUid,"%s",Z_STRVAL_PP(uid));
	}else if(IS_LONG == Z_TYPE_PP(uid)){
		sprintf(trueUid,"%d",Z_LVAL_PP(uid));
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}

	if(type == 1){
		add_assoc_string(gatewayUidSessionList,trueUid,Z_STRVAL_PP(sessionId),1);
		writeLogs("[CTcpGateway] worker call bindUser now uid-session list is len:[%d] \n",zend_hash_num_elements(Z_ARRVAL_P(gatewayUidSessionList)));
	}else{
		zend_hash_del(Z_ARRVAL_P(gatewayUidSessionList),trueUid,strlen(trueUid)+1);
		writeLogs("[CTcpGateway] worker call unBindUser now uid-session list is len:[%d] \n",zend_hash_num_elements(Z_ARRVAL_P(gatewayUidSessionList)));
	}

	char	callbackInfo[400],
			*gatewayMessage;

	sprintf(callbackInfo,"{\"eventId\":\"%s\",\"data\":{\"result\":true,\"sessionId\":\"%s\"}}",Z_STRVAL_PP(eventId),Z_STRVAL_PP(sessionId));
	createMessage("bindUid",callbackInfo,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	gatewaySendMessageToSocket(fd,gatewayMessage,object TSRMLS_CC);



	efree(gatewayMessage);
	zval_ptr_dtor(&eventJson);
}

void gatewayClearUserDisconnect(char *userSessionId){

	//ɾ��session��workerӳ���¼
	zend_hash_del(Z_ARRVAL_P(gatewayUserSessionList),userSessionId,strlen(userSessionId)+1);

	//ɾ����session�����Ⱥ��
	int		i,h,j,k;
	zval	**thisGroupUser,
			**thisUidSessionId;
	char	*key;
	ulong	uKey;

	h = zend_hash_num_elements(Z_ARRVAL_P(gatewayGroupList));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(gatewayGroupList));
	for(i = 0 ; i < h ; i++){
		zend_hash_get_current_data(Z_ARRVAL_P(gatewayGroupList),(void**)&thisGroupUser);


		if(IS_ARRAY == Z_TYPE_PP(thisGroupUser) && zend_hash_exists(Z_ARRVAL_PP(thisGroupUser),userSessionId,strlen(userSessionId)+1) ){
			zend_hash_del(Z_ARRVAL_PP(thisGroupUser),userSessionId,strlen(userSessionId)+1);
		}
		zend_hash_move_forward(Z_ARRVAL_P(gatewayGroupList));
	}

	//ɾ��uid��session�� gatewayUidSessionList
	h = zend_hash_num_elements(Z_ARRVAL_P(gatewayUidSessionList));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(gatewayUidSessionList));
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(gatewayUidSessionList));
	for(i = 0 ; i < h ; i++){
		if(SUCCESS == zend_hash_get_current_data(Z_ARRVAL_P(gatewayUidSessionList),(void**)&thisUidSessionId) && IS_STRING == Z_TYPE_PP(thisUidSessionId) && strcmp(Z_STRVAL_PP(thisUidSessionId),userSessionId) == 0 ){
			zend_hash_get_current_key(Z_ARRVAL_P(gatewayUidSessionList),&key,&uKey,0);
			zend_hash_del(Z_ARRVAL_P(gatewayUidSessionList),key,strlen(key)+1);
		}
		zend_hash_move_forward(Z_ARRVAL_P(gatewayUidSessionList));
	}


	//ɾ����session�󶨵�sessionData

}

//����session
void gatewayEditSession(int fd,char *message,int type,zval *object TSRMLS_DC)
{

	//��ȡ�¼�id
	zval	*eventJson,
			**uid,
			**sessionId,
			**thisGroup,
			**eventId;

	json_decode(message,&eventJson);

	//eventId
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"eventId",8,(void**)&eventId) && IS_STRING == Z_TYPE_PP(eventId)){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}

	if(	SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"uid",strlen("uid")+1,(void**)&uid) && 
		SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"clientSessionId",strlen("clientSessionId")+1,(void**)&sessionId) && IS_STRING == Z_TYPE_PP(sessionId) ){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}




	zval_ptr_dtor(&eventJson);
}

void gatewayForwardMessage(int fd,char *message,zval *object TSRMLS_DC){

	zval	*eventJson,
			**toUid,
			**eventId,
			**event,
			**sendMessage,
			**sessionId,
			**workerFd;

	json_decode(message,&eventJson);

	if(	SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"toUid",strlen("toUid")+1,(void**)&toUid) && 
		SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"eventId",strlen("eventId")+1,(void**)&eventId) && IS_STRING == Z_TYPE_PP(eventId) && 
		SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"event",strlen("event")+1,(void**)&event) && IS_STRING == Z_TYPE_PP(event) && 
		SUCCESS == zend_hash_find(Z_ARRVAL_P(eventJson),"message",strlen("message")+1,(void**)&sendMessage)
	){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}

	//Ѱ��toUid ��session
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(gatewayUidSessionList),Z_STRVAL_PP(toUid),strlen(Z_STRVAL_PP(toUid))+1,(void**)&sessionId) && IS_STRING == Z_TYPE_PP(sessionId) ){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}

	//Ѱ��session���ڵ�worker��fd
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(gatewayUserSessionList),Z_STRVAL_PP(sessionId),strlen(Z_STRVAL_PP(sessionId))+1,(void**)&workerFd) && IS_LONG == Z_TYPE_PP(workerFd) ){
	}else{
		zval_ptr_dtor(&eventJson);
		return;
	}


	//��toUid����worker�ظ���Ϣ Ҫ��ת����Ϣ
	char	*gatewayMessage;
	createMessage("forwardMessage",message,MSG_USER,&gatewayMessage,3 TSRMLS_CC);
	gatewaySendMessageToSocket(Z_LVAL_PP(workerFd),gatewayMessage,object TSRMLS_CC);
	efree(gatewayMessage);


	//�ظ���ѯ�� �����ɹ��ص�
	char	*rebackMessage,
			callbackInfo[200];
	sprintf(callbackInfo,"{\"eventId\":\"%s\",\"data\":{\"result\":true,\"sessionId\":\"%s\"}}",Z_STRVAL_PP(eventId),Z_STRVAL_PP(sessionId));
	createMessage("forwardMessageSuccess",callbackInfo,MSG_USER,&rebackMessage,3 TSRMLS_CC);
	gatewaySendMessageToSocket(fd,rebackMessage,object TSRMLS_CC);
	efree(rebackMessage);

	zval_ptr_dtor(&eventJson);
}


void processGatewayMessage(int fd,char *thisMessage,zval *object TSRMLS_DC){

	char	*base64Decoder;
	zval	*jsonDecoder,
			**type,
			**message,
			**sessionId;

	base64Decode(thisMessage,&base64Decoder);
	json_decode(base64Decoder,&jsonDecoder);

	if(IS_ARRAY != Z_TYPE_P(jsonDecoder) || zend_hash_num_elements(Z_ARRVAL_P(jsonDecoder)) != 4){
		efree(base64Decoder);
		zval_ptr_dtor(&jsonDecoder);
		return;
	}

	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),1,(void**)&type);
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),2,(void**)&message);
	zend_hash_index_find(Z_ARRVAL_P(jsonDecoder),3,(void**)&sessionId);

	//worker ע��
	if(strcmp(Z_STRVAL_PP(type),"serverRegister") == 0){
		//��¼socket gatewaySocketList
		zval	*sessionData,
				*nowTime;
		microtime(&nowTime);
		MAKE_STD_ZVAL(sessionData);
		array_init(sessionData);
		add_index_long(sessionData,0,fd);
		add_index_double(sessionData,1,Z_DVAL_P(nowTime));
		zval_ptr_dtor(&nowTime);
		add_assoc_zval(gatewaySessionList,Z_STRVAL_PP(sessionId),sessionData);
		add_index_string(gatewaySocketList,fd,Z_STRVAL_PP(sessionId),1);
		writeLogs("[CTcpGateway] worker[%s] is register success...\n",Z_STRVAL_PP(sessionId));
	}

	//socket����
	if(strcmp(Z_STRVAL_PP(type),"connect") == 0){

		//��ȡsession���浽  gatewayUserSessionList
		zval	*messageContent,
				**clientSessionId;
		json_decode(Z_STRVAL_PP(message),&messageContent);
		if(SUCCESS == zend_hash_find(Z_ARRVAL_P(messageContent),"sessionId",strlen("sessionId")+1,(void**)&clientSessionId) ){
			//����sessionӳ���¼
			add_assoc_long(gatewayUserSessionList,Z_STRVAL_PP(clientSessionId),fd);
		}
		zval_ptr_dtor(&messageContent);
		writeLogs("[CTcpGateway] User connect,now session num is [%d] ...\n",zend_hash_num_elements(Z_ARRVAL_P(gatewayUserSessionList)));
	}

	//socket����
	if(strcmp(Z_STRVAL_PP(type),"disconnect") == 0){

		//��ȡsession ��gatewayUserSessionList�Ƴ�
		zval	*messageContent,
				**clientSessionId;
		json_decode(Z_STRVAL_PP(message),&messageContent);
		if(SUCCESS == zend_hash_find(Z_ARRVAL_P(messageContent),"sessionId",strlen("sessionId")+1,(void**)&clientSessionId) ){
			
			//ɾ�����ذ󶨵�����
			gatewayClearUserDisconnect(Z_STRVAL_PP(clientSessionId));
		}

		zval_ptr_dtor(&messageContent);
		writeLogs("[CTcpGateway] User disconnect,now session num is [%d] ...\n",zend_hash_num_elements(Z_ARRVAL_P(gatewayUserSessionList)));
	}

	//��Ϣ�㲥
	if(strcmp(Z_STRVAL_PP(type),"broadcast") == 0){
	
		//����� thisMessage ת�����������ߵ�woker
		gatewayBroadcastMessage(thisMessage,object TSRMLS_CC);
	
	}

	//��ѯ��������
	if(strcmp(Z_STRVAL_PP(type),"getAllConnection") == 0){
		gatewayResponseAllSession(fd,Z_STRVAL_PP(message),object TSRMLS_CC);
	}

	//�ͻ��˰�Ⱥ��
	if(strcmp(Z_STRVAL_PP(type),"joinGroup") == 0){
		gatewayEditGroup(fd,Z_STRVAL_PP(message),1,object TSRMLS_CC);
	}

	//�ͻ��˽��Ⱥ��
	if(strcmp(Z_STRVAL_PP(type),"leaveGroup") == 0){
		gatewayEditGroup(fd,Z_STRVAL_PP(message),2,object TSRMLS_CC);
	}

	//��ȡȺ����Ϣ
	if(strcmp(Z_STRVAL_PP(type),"getGroup") == 0){
		gatewayGetGroup(fd,Z_STRVAL_PP(message),object TSRMLS_CC);
	}

	//��UID
	if(strcmp(Z_STRVAL_PP(type),"bindUid") == 0){
		gatewayEditUid(fd,Z_STRVAL_PP(message),1,object TSRMLS_CC);
	}

	//���UID
	if(strcmp(Z_STRVAL_PP(type),"unBindUid") == 0){
		gatewayEditUid(fd,Z_STRVAL_PP(message),2,object TSRMLS_CC);
	}

	//����session gatewaySessionData
	if(strcmp(Z_STRVAL_PP(type),"setSession") == 0){
		gatewayEditSession(fd,Z_STRVAL_PP(message),1,object TSRMLS_CC);
	}

	//ת����Ϣ
	if(strcmp(Z_STRVAL_PP(type),"forwardMessage") == 0){
		gatewayForwardMessage(fd,Z_STRVAL_PP(message),object TSRMLS_CC);
	}


	efree(base64Decoder);
	zval_ptr_dtor(&jsonDecoder);

}

//��⵽worker����
void gatewayCloseWokerSocket(int fd){

	//�ҳ�worker session
	zval	**sessionId;
	if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(gatewaySocketList),fd,(void**)&sessionId) && IS_STRING == Z_TYPE_PP(sessionId) ){

		writeLogs("[CTcpGateway] worker[%s] is unRegister...\n",Z_STRVAL_PP(sessionId));

		//ɾ��session
		if(zend_hash_exists(Z_ARRVAL_P(gatewaySessionList),Z_STRVAL_PP(sessionId),strlen(Z_STRVAL_PP(sessionId))+1) ){
			zend_hash_del(Z_ARRVAL_P(gatewaySessionList),Z_STRVAL_PP(sessionId),strlen(Z_STRVAL_PP(sessionId))+1);
		}
		zend_hash_index_del(Z_ARRVAL_P(gatewaySocketList),fd);
	}

	if(zend_hash_index_exists(Z_ARRVAL_P(gatewaySendList),fd)){
		zend_hash_index_del(Z_ARRVAL_P(gatewaySendList),fd);
	}

	close(fd);

}

//���ط����� ÿ30����һ�� ��worker״̬
void checkServerSocketStatus(){

	int num = zend_hash_num_elements(Z_ARRVAL_P(gatewaySessionList));
	writeLogs("[CTcpGateway] begin to check worker status, now worker num is [%d]...\n",num);



}

void gateWaytimerCallback(){

	int timestamp = getMicrotime();

	if(timestamp - gatewayLastCheckTime >= 30){

		checkServerSocketStatus();

		gatewayLastCheckTime = getMicrotime();
	}
}

PHP_METHOD(CTcpGateway,listen)
{
	zval	*host,
			*port,
			*object,
			**argv,
			**SERVER,
			*pidList;

	int		errorCode = 0,
			isDaemon = 0;

	char	appPath[2024],
			codePath[2024];

	RETVAL_ZVAL(getThis(),1,0);

	ini_seti("memory_limit",-1);

	host = zend_read_property(CTcpGatewayCe,getThis(),ZEND_STRL("host"), 0 TSRMLS_CC);
	port = zend_read_property(CTcpGatewayCe,getThis(),ZEND_STRL("port"), 0 TSRMLS_CC);

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
			zend_hash_move_forward(Z_ARRVAL_PP(argv));
		}
	}

	//�����ź� ��ֹ�����˳�
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);

	//daemon
	if(isDaemon){
		writeLogs("run as a daemon process..\n");
		int s = daemon(1, 0);
	}

	if(gatewaySocketList == NULL){
		MAKE_STD_ZVAL(gatewaySocketList);
		array_init(gatewaySocketList);
	}
	if(gatewaySessionList == NULL){
		MAKE_STD_ZVAL(gatewaySessionList);
		array_init(gatewaySessionList);
	}
	if(gatewayUserSessionList == NULL){
		MAKE_STD_ZVAL(gatewayUserSessionList);
		array_init(gatewayUserSessionList);
	}
	if(gatewaySendList == NULL){
		MAKE_STD_ZVAL(gatewaySendList);
		array_init(gatewaySendList);
	}
	if(gatewayGroupList == NULL){
		MAKE_STD_ZVAL(gatewayGroupList);
		array_init(gatewayGroupList);
	}
	if(gatewayUidSessionList == NULL){
		MAKE_STD_ZVAL(gatewayUidSessionList);
		array_init(gatewayUidSessionList);
	}
	if(gatewaySessionData == NULL){
		MAKE_STD_ZVAL(gatewaySessionData);
		array_init(gatewaySessionData);
	}

	//����socket�׽��� ����fork�ӽ��� �Թ�����׽���
	int listenSocket = startTcpListen(Z_STRVAL_P(host),Z_LVAL_P(port));
	if(listenSocket < 0){
		//�����쳣
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[FatalException] TCP Gateway can not bind %s:%d ",Z_STRVAL_P(host),Z_LVAL_P(port));
		return;
	}

	//����epoll
	int epfd = epoll_create(1024);
	if(epfd <= 0){
		php_error_docref(NULL TSRMLS_CC, E_ERROR ,"[FatalException] TCP Gateway can not create epoll fd");
		return;
	}

	//�󶨶�����
	struct epoll_event  ev;
    ev.events = EPOLLIN; 
    ev.data.fd = listenSocket;
	epoll_ctl(epfd,EPOLL_CTL_ADD,listenSocket,&ev);

	struct epoll_event revs[128];
    int n = sizeof(revs)/sizeof(revs[0]);
    int timeout = 3000;
    int i,num = 0;

	//��ʼ��������
	while(1){

		//��ʼepoll�¼��ȴ�
       num = epoll_wait(epfd,revs,n,500);

	   //�������״̬
	   gateWaytimerCallback();

	   //�ȼ�鷢����Ϣ����
	   doGatewaySendMessageList();

	   for(i = 0 ; i < num; i++){

			int fd = revs[i].data.fd;

			// ����accept����������
			if( fd == listenSocket && (revs[i].events & EPOLLIN) ){

				struct sockaddr_in client;
				socklen_t len = sizeof(client);
				extern int errno;
				int new_sock = accept(fd,(struct sockaddr *)&client,&len);

				if( new_sock < 0 ){
					if(errno == 11){
					}else{
						setLogs("CTcpGateway [%d] accept fail,errorno:%d \n",getpid(),errno);
					}
					continue;
				}

				//socket��Ϊ������
				fcntl(new_sock, F_SETFL, fcntl(new_sock, F_GETFL, NULL) | O_NONBLOCK);

				//�¼�����Ϣ
				ev.events = EPOLLIN;
				ev.data.fd = new_sock;
				epoll_ctl(epfd,EPOLL_CTL_ADD,new_sock,&ev);
				continue;
			}

			if(revs[i].events & EPOLLIN)	{

				char		buf[2],
							*thisMessage;
				int			readLen = 0,k;

				smart_str	tempBuffer[10240] = {0};

				while(1){
					errno = 0;
					readLen = read(fd,buf,sizeof(buf)-1);

					if(readLen <= 0){
						if(readLen == 0){

							smart_str_0(&tempBuffer[fd]);
							smart_str_free(&tempBuffer[fd]);

							epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
							gatewayCloseWokerSocket(fd);
						}
						break;
					}

					if(buf[0] != '\n'){
						smart_str_appendc(&tempBuffer[fd],buf[0]);
					}else{
						smart_str_0(&tempBuffer[fd]);
						thisMessage = estrdup(tempBuffer[fd].c);
						smart_str_free(&tempBuffer[fd]);
						processGatewayMessage(fd,thisMessage,object TSRMLS_CC);
						efree(thisMessage);
						break;
					}	
				}
			}
	   }

	}

}

PHP_METHOD(CTcpGateway,on)
{}

PHP_METHOD(CTcpGateway,onData)
{}

PHP_METHOD(CTcpGateway,onError)
{}


#endif