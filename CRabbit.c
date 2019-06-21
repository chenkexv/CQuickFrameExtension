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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"


#include "php_CQuickFramework.h"
#include "php_CRabbit.h"
#include "php_CException.h"

//zend�෽��
zend_function_entry CRabbit_functions[] = {
	PHP_ME(CRabbit,getInstance,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRabbit,__construct,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(CRabbit,__destruct,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_DTOR)
	PHP_ME(CRabbit,bind,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRabbit,get,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRabbit,ack,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRabbit,llen,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRabbit,getQueue,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRabbit,getExchange,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRabbit,publish,NULL,ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

//ģ�鱻����ʱ
CMYFRAME_REGISTER_CLASS_RUN(CRabbit)
{
	zend_class_entry funCe;

	INIT_CLASS_ENTRY(funCe,"CRabbit",CRabbit_functions);
	CRabbitCe = zend_register_internal_class(&funCe TSRMLS_CC);

	zend_declare_property_null(CRabbitCe, ZEND_STRL("instance"),ZEND_ACC_PRIVATE|ZEND_ACC_STATIC TSRMLS_CC);

	//������
	zend_declare_property_null(CRabbitCe, ZEND_STRL("_conn"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CRabbitCe, ZEND_STRL("_channel"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CRabbitCe, ZEND_STRL("_configName"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CRabbitCe, ZEND_STRL("_exchange"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CRabbitCe, ZEND_STRL("_bindExchange"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CRabbitCe, ZEND_STRL("_queueList"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CRabbitCe, ZEND_STRL("_exchangeName"),"default",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CRabbitCe, ZEND_STRL("_bindRoute"),"defaultRoute",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CRabbitCe, ZEND_STRL("_bindQueue"),"defaultQueue",ZEND_ACC_PRIVATE TSRMLS_CC);

	return SUCCESS;
}

void CRabbit_getInstance(zval **returnZval,char *groupName TSRMLS_DC)
{
	zval	*instanceZval,
			**instaceSaveZval;

	int hasExistConfig;

	//��ȡ��̬instace����
	instanceZval = zend_read_static_property(CRabbitCe,ZEND_STRL("instance"),0 TSRMLS_CC);

	//���ΪNULL�����ΪZvalHashtable
	if(IS_ARRAY != Z_TYPE_P(instanceZval)){
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		zend_update_static_property(CRabbitCe,ZEND_STRL("instance"),saveArray TSRMLS_CC);
		zval_ptr_dtor(&saveArray);
		instanceZval = zend_read_static_property(CRabbitCe,ZEND_STRL("instance"),0 TSRMLS_CC);
	}

	//�жϵ��ж����д���config��key
	hasExistConfig = zend_hash_exists(Z_ARRVAL_P(instanceZval), groupName, strlen(groupName)+1);

	//Ϊ��ʱ��ʵ��������
	if(0 == hasExistConfig ){
		

		zval			*object,
						*saveObject;


		//ʵ�����ò��
		MAKE_STD_ZVAL(object);
		object_init_ex(object,CRabbitCe);

		//ִ�й�����
		if (CRabbitCe->constructor) {
			zval constructReturn;
			zval constructVal,
				 params1;
			zval *paramsList[1];
			paramsList[0] = &params1;
			MAKE_STD_ZVAL(paramsList[0]);
			ZVAL_STRING(paramsList[0],groupName,1);
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal, CRabbitCe->constructor->common.function_name, 0);
			call_user_function(NULL, &object, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
			zval_ptr_dtor(&paramsList[0]);
			zval_dtor(&constructReturn);
		}

		//�ж��Ƿ��б����쳣
		if(EG(exception)){
			MAKE_STD_ZVAL(*returnZval);
			zval_ptr_dtor(&object);
			return;
		}

		//������󱣴���instance��̬����
		MAKE_STD_ZVAL(saveObject);
		ZVAL_ZVAL(saveObject,object,1,0);

		zend_hash_add(Z_ARRVAL_P(instanceZval),groupName,strlen(groupName)+1,&saveObject,sizeof(zval*),NULL);
		zend_update_static_property(CRabbitCe, ZEND_STRL("instance"),instanceZval TSRMLS_CC);

		//����
		MAKE_STD_ZVAL(*returnZval);
		ZVAL_ZVAL(*returnZval,object,1,0);
		zval_ptr_dtor(&object);
		return;
	}

	//ֱ��ȡinstace��̬�����еķ���ֵ
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(instanceZval),groupName,strlen(groupName)+1,(void**)&instaceSaveZval) ){
		MAKE_STD_ZVAL(*returnZval);
		ZVAL_ZVAL(*returnZval,*instaceSaveZval,1,0);
		return;
	}

	zend_throw_exception(CQueueExceptionCe, "[CQueueException] An internal error occurred while CQuickFramework was acquired by AMQP ", 1 TSRMLS_CC);
}


PHP_METHOD(CRabbit,getInstance)
{
	zval *instanceZval;
	char *groupName = "main";
	int groupNameLen = 0;

	//�ж��Ƿ�ָ������
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|s",&groupName,&groupNameLen) == FAILURE){
		zend_throw_exception(CQueueExceptionCe, "[CQueueException]The server is not currently installed AMQP extension", 1 TSRMLS_CC);
		return;
	}

	CRabbit_getInstance(&instanceZval,groupName TSRMLS_CC);
	ZVAL_ZVAL(return_value,instanceZval,1,0);
	zval_ptr_dtor(&instanceZval);
}

//��ȡRABBIT����
void CRabbit_geConnection(zval *object,char *groupName TSRMLS_DC)
{

	zval	*cconfigInstanceZval,
			*connectParamsZval,
			*ampqConnectObjectZval,
			*amqpChannelObjectZval;

	char	thisConfigKey[1024];

	zend_class_entry	**amqpConnectPP,
						*amqpConnectCe,
						**amqpChannelPP,
						*ampqChannelCe;


	//��ȡ���ö���
	CConfig_getInstance("main",&cconfigInstanceZval TSRMLS_CC);
	
	//��ǰ���õ�����
	sprintf(thisConfigKey,"%s%s%s","RABBITMQ.",groupName,".connection");

	//��ȡ��������
	CConfig_load(thisConfigKey,cconfigInstanceZval,&connectParamsZval TSRMLS_CC);
	if(IS_ARRAY != Z_TYPE_P(connectParamsZval)){
		char errorMessage[1024];
		sprintf(errorMessage,"%s%s","[CQueueException] There is no specified MQ configuration : ",thisConfigKey);
		zval_ptr_dtor(&cconfigInstanceZval);
		zval_ptr_dtor(&connectParamsZval);
		zend_throw_exception(CQueueExceptionCe, errorMessage, 6 TSRMLS_CC);
		return;
	}

	//��ȡAMQP����
	if(zend_hash_find(EG(class_table),"amqpconnection",strlen("amqpconnection")+1,(void**)&amqpConnectPP ) == FAILURE){
		zval_ptr_dtor(&cconfigInstanceZval);
		zval_ptr_dtor(&connectParamsZval);
		zend_throw_exception(CQueueExceptionCe, "[CQueueException]The server is not currently installed AMQP extension", 1 TSRMLS_CC);
		return;
	}

	amqpConnectCe = *amqpConnectPP;
	MAKE_STD_ZVAL(ampqConnectObjectZval);
	object_init_ex(ampqConnectObjectZval,amqpConnectCe);

	//���Ե��ù�����
	if (amqpConnectCe->constructor) {
		zval	constructReturn,
				constructVal,
				*paramsList[1],
				params1;

		paramsList[0] = &params1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_ZVAL(paramsList[0],connectParamsZval,1,0);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal, amqpConnectCe->constructor->common.function_name, 0);
		call_user_function(NULL, &ampqConnectObjectZval, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_dtor(&constructReturn);
	}

	//������connect����
	MODULE_BEGIN
		zval	constructReturn,
				constructVal;
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"pconnect", 0);
		call_user_function(NULL, &ampqConnectObjectZval, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
		zval_dtor(&constructReturn);

		if(EG(exception)){
			char errMessage[1024];
			sprintf(errMessage,"%s%s","[CQueueException] Unable to connect to the Rabbit server : ",thisConfigKey);
			Z_OBJ_HANDLE_P(EG(exception)) = 0;
			zend_clear_exception(TSRMLS_C);
			zval_ptr_dtor(&cconfigInstanceZval);
			zval_ptr_dtor(&connectParamsZval);
			zval_ptr_dtor(&ampqConnectObjectZval);
			zend_throw_exception(CQueueExceptionCe, errMessage, 2 TSRMLS_CC);
			return;
		}
	MODULE_END

	//���浽����
	zend_update_property(CRabbitCe,object,ZEND_STRL("_conn"),ampqConnectObjectZval TSRMLS_CC);

	//ͬʱ��ʼ��channel����
	if(zend_hash_find(EG(class_table),"amqpchannel",strlen("amqpchannel")+1,(void**)&amqpChannelPP ) == FAILURE){
		zval_ptr_dtor(&cconfigInstanceZval);
		zval_ptr_dtor(&connectParamsZval);
		zval_ptr_dtor(&ampqConnectObjectZval);
		zend_throw_exception(CQueueExceptionCe, "[CQueueException]The server is not currently installed AMQP extension", 1 TSRMLS_CC);
		return;
	}

	ampqChannelCe = *amqpChannelPP;
	MAKE_STD_ZVAL(amqpChannelObjectZval);
	object_init_ex(amqpChannelObjectZval,ampqChannelCe);

	//���ù����� �������Ӷ���
	if (ampqChannelCe->constructor) {
		zval	constructReturn,
				constructVal,
				*paramsList[1],
				params1;

		paramsList[0] = &params1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_ZVAL(paramsList[0],ampqConnectObjectZval,1,0);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal, ampqChannelCe->constructor->common.function_name, 0);
		call_user_function(NULL, &amqpChannelObjectZval, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_dtor(&constructReturn);
	}

	//���浽����
	zend_update_property(CRabbitCe,object,ZEND_STRL("_channel"),amqpChannelObjectZval TSRMLS_CC);


	//���ٱ���
	zval_ptr_dtor(&cconfigInstanceZval);
	zval_ptr_dtor(&connectParamsZval);
	zval_ptr_dtor(&ampqConnectObjectZval);
	zval_ptr_dtor(&amqpChannelObjectZval);

}


PHP_METHOD(CRabbit,__construct)
{
	char *groupName;
	int groupNameLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&groupName,&groupNameLen) == FAILURE){
		zend_throw_exception(CRedisExceptionCe, "[CQueueException] CRabbit execution method parameter error", 12012 TSRMLS_CC);
		return;
	}

	CRabbit_geConnection(getThis(),groupName TSRMLS_CC);
}

PHP_METHOD(CRabbit,__destruct)
{
	zval *connectZval;

	connectZval = zend_read_property(CRabbitCe,getThis(),ZEND_STRL("_conn"),0 TSRMLS_CC);

	//Ϊ����ʱ������رպ���
	if(IS_OBJECT == Z_TYPE_P(connectZval)){

		zval	constructReturn,
				constructVal;

		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"disconnect", 0);
		call_user_function(NULL, &connectZval, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
		zval_dtor(&constructReturn);
	}
}

PHP_METHOD(CRabbit,bind)
{

	char	*queue,
			*route;
	int		queueLen = 0,
			routeLen = 0;

	//��ȡ����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ss",&route,&routeLen,&queue,&queueLen) == FAILURE){
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Call [CRabbit->bind] Parameter error ", 10001 TSRMLS_CC);
		return;
	}

	if(routeLen >0 ){
		zend_update_property_string(CRabbitCe,getThis(),ZEND_STRL("_bindRoute"),route TSRMLS_CC);
	}

	if(queueLen >0 ){
		zend_update_property_string(CRabbitCe,getThis(), ZEND_STRL("_bindQueue"),queue TSRMLS_CC);
	}

	RETVAL_ZVAL(getThis(),1,0);
}

//��ȡ����
void CRabbit_getQueue(zval *object,char *queue,int tags,zval **returnObject TSRMLS_DC){

	zval			*queueListZval,
					*amqpQueueObject,
					**existObject;

	zend_class_entry	**amqpqueuePP,
						*amqpqueueCe;

	MAKE_STD_ZVAL(*returnObject);
	ZVAL_NULL(*returnObject);

	//��������
	queueListZval = zend_read_property(CRabbitCe,object,ZEND_STRL("_queueList"),0 TSRMLS_CC);
	if(IS_NULL == Z_TYPE_P(queueListZval)){
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		zend_update_property(CRabbitCe,object,ZEND_STRL("_queueList"),saveArray TSRMLS_CC);
		zval_ptr_dtor(&saveArray);
		queueListZval = zend_read_property(CRabbitCe,object,ZEND_STRL("_queueList"),0 TSRMLS_CC);
	}

	//�ж��Ƿ��Ѵ��ڶ���
	if(0 == zend_hash_exists(Z_ARRVAL_P(queueListZval),queue,strlen(queue)+1)){

		zval	*channelObjectZval;

		//��ȡchannelObject
		channelObjectZval = zend_read_property(CRabbitCe,object,ZEND_STRL("_channel"),0 TSRMLS_CC);

		//��ȡAMQPQueue����
		if(zend_hash_find(EG(class_table),"amqpqueue",strlen("amqpqueue")+1,(void**)&amqpqueuePP ) == FAILURE){
			zend_throw_exception(CQueueExceptionCe, "[CQueueException]The server is not currently installed AMQP extension", 1 TSRMLS_CC);
			return;
		}
		
		//��ȡ����
		amqpqueueCe = *amqpqueuePP;
		MAKE_STD_ZVAL(amqpQueueObject);
		object_init_ex(amqpQueueObject,amqpqueueCe);

		//���ù����� �������Ӷ���
		if (amqpqueueCe->constructor) {
			zval	constructReturn,
					constructVal,
					*paramsList[1],
					params1;

			paramsList[0] = &params1;
			MAKE_STD_ZVAL(paramsList[0]);
			ZVAL_ZVAL(paramsList[0],channelObjectZval,1,0);
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal, amqpqueueCe->constructor->common.function_name, 0);
			call_user_function(NULL, &amqpQueueObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
			zval_ptr_dtor(&paramsList[0]);
			zval_dtor(&constructReturn);
		}

		//����setName����
		MODULE_BEGIN
			zval	constructReturn,
					constructVal,
					*paramsList[1],
					params1;
			paramsList[0] = &params1;
			MAKE_STD_ZVAL(paramsList[0]);
			ZVAL_STRING(paramsList[0],queue,1);
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal,"setName", 0);
			call_user_function(NULL, &amqpQueueObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
			zval_ptr_dtor(&paramsList[0]);
			zval_dtor(&constructReturn);
		MODULE_END

		//Tag��Ϊ0ʱ�����setTags
		if(0 != tags){
			zval	constructReturn,
					constructVal,
					*paramsList[1],
					params1;
			paramsList[0] = &params1;
			MAKE_STD_ZVAL(paramsList[0]);
			ZVAL_LONG(paramsList[0],tags);
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal,"setFlags", 0);
			call_user_function(NULL, &amqpQueueObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
			zval_ptr_dtor(&paramsList[0]);
			zval_dtor(&constructReturn);
		}

		//����declare
		MODULE_BEGIN
			zval	constructReturn,
					constructVal;
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal,"declare", 0);
			call_user_function(NULL, &amqpQueueObject, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
			zval_dtor(&constructReturn);
		MODULE_END

		//���浽������
		add_assoc_zval(queueListZval,queue,amqpQueueObject);
		zend_update_property(CRabbitCe,object,ZEND_STRL("_queueList"),queueListZval TSRMLS_CC);
		ZVAL_ZVAL(*returnObject,amqpQueueObject,1,0);
		return;
	}

	//ֱ�Ӷ�ȡHash������
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(queueListZval),queue,strlen(queue)+1,(void**)&existObject) ){
		ZVAL_ZVAL(*returnObject,*existObject,1,0);
	}
}

PHP_METHOD(CRabbit,get)
{

	char	*route,
			*queue,
			*newRoute,
			*newQueue;

	int		routeLen = 0,
			queueLen = 0,
			autoAck = 0,
			flags = 0;

	zval	*queueObject = NULL,
			*bindRouteZval,
			*bindQueueZval,
			*exchangeZval,
			*queueReturnZval;

	//��ȡ����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|ssbl",&route,&routeLen,&queue,&queueLen,&autoAck,&flags) == FAILURE){
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Call [CRabbit->get] Parameter error ", 10001 TSRMLS_CC);
		return;
	}

	//��ֵ
	if(routeLen >0 ){
		zend_update_property_string(CRabbitCe,getThis(),ZEND_STRL("_bindRoute"),route TSRMLS_CC);
	}
	if(queueLen >0 ){
		zend_update_property_string(CRabbitCe,getThis(), ZEND_STRL("_bindQueue"),queue TSRMLS_CC);
	}

	//��ȡ_bindRoute��_bindQueue
	bindRouteZval = zend_read_property(CRabbitCe,getThis(),ZEND_STRL("_bindRoute"),0 TSRMLS_CC);
	bindQueueZval = zend_read_property(CRabbitCe,getThis(),ZEND_STRL("_bindQueue"),0 TSRMLS_CC);
	exchangeZval = zend_read_property(CRabbitCe,getThis(),ZEND_STRL("_exchangeName"),0 TSRMLS_CC);

	//��ȡ����
	CRabbit_getQueue(getThis(),Z_STRVAL_P(bindQueueZval),flags,&queueObject TSRMLS_CC);
	if(IS_OBJECT != Z_TYPE_P(queueObject)){
		zval_ptr_dtor(&queueObject);
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Get Rabbit Queue Object Fail ", 8 TSRMLS_CC);
		return;
	}

	//���ö��е�bind����
	MODULE_BEGIN
		zval	constructReturn,
				constructVal,
				*paramsList[2],
				params1,
				params2;
		paramsList[0] = &params1;
		paramsList[1] = &params2;
		MAKE_STD_ZVAL(paramsList[0]);
		MAKE_STD_ZVAL(paramsList[1]);
		ZVAL_STRING(paramsList[0],Z_STRVAL_P(exchangeZval),1);
		ZVAL_STRING(paramsList[1],Z_STRVAL_P(bindRouteZval),1);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"bind", 0);
		call_user_function(NULL, &queueObject, &constructVal, &constructReturn, 2, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_ptr_dtor(&paramsList[1]);
		zval_dtor(&constructReturn);
	MODULE_END


	//����queue�����get����
	if(1 == autoAck){
		
		zval	constructReturn,
				constructVal,
				*paramsList[2],
				params1;
		paramsList[0] = &params1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_LONG(paramsList[0],128);	//���� AMQP_AUTOACK
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"get", 0);
		call_user_function(NULL, &queueObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);

		//���ض���
		queueReturnZval = &constructReturn;
		ZVAL_ZVAL(return_value,queueReturnZval,1,0);
		zval_dtor(&constructReturn);
	}else{

		zval	constructReturn,
				constructVal;
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"get", 0);
		call_user_function(NULL, &queueObject, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);

		//���ض���
		queueReturnZval = &constructReturn;
		ZVAL_ZVAL(return_value,queueReturnZval,1,0);
		zval_dtor(&constructReturn);
	}


	//���ٶ���
	zval_ptr_dtor(&queueObject);

}

PHP_METHOD(CRabbit,ack)
{
	zval	*tagsZval = NULL,
			*queueObject,
			*bindRouteZval,
			*bindQueueZval,
			*exchangeZval;

	char	*route,
			*queue;

	int		routeLen = 0,
			queueLen = 0,
			tagId = 0;


	//��ȡ����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z|ss",&tagsZval,&route,&routeLen,&queue,&queueLen) == FAILURE){
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Call [CRabbit->ack] Parameter error ", 10002 TSRMLS_CC);
		return;
	}

	//û������
	if(IS_OBJECT == Z_TYPE_P(tagsZval) || IS_LONG == Z_TYPE_P(tagsZval)){
	}else{
		RETURN_FALSE;
	}

	//��ֵ
	if(routeLen >0 ){
		zend_update_property_string(CRabbitCe,getThis(),ZEND_STRL("_bindRoute"),route TSRMLS_CC);
	}
	if(queueLen >0 ){
		zend_update_property_string(CRabbitCe,getThis(), ZEND_STRL("_bindQueue"),queue TSRMLS_CC);
	}

	//��ȡ_bindRoute��_bindQueue
	bindRouteZval = zend_read_property(CRabbitCe,getThis(),ZEND_STRL("_bindRoute"),0 TSRMLS_CC);
	bindQueueZval = zend_read_property(CRabbitCe,getThis(),ZEND_STRL("_bindQueue"),0 TSRMLS_CC);
	exchangeZval = zend_read_property(CRabbitCe,getThis(),ZEND_STRL("_exchangeName"),0 TSRMLS_CC);

	//��ȡTagsString ��tagsΪ��������ö��󷽷�
	if(IS_OBJECT == Z_TYPE_P(tagsZval)){
		zval	constructReturn,
				constructVal;
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"getDeliveryTag", 0);
		call_user_function(NULL, &tagsZval, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
		tagId = Z_LVAL(constructReturn);
		zval_dtor(&constructReturn);
	}else if(IS_LONG == Z_TYPE_P(tagsZval)){
		tagId = Z_LVAL_P(tagsZval);
	}else{
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Call [CRabbit->ack] Parameter error ", 10003 TSRMLS_CC);
		return;
	}

	//��ȡ����
	CRabbit_getQueue(getThis(),Z_STRVAL_P(bindQueueZval),0,&queueObject TSRMLS_CC);
	if(IS_OBJECT != Z_TYPE_P(queueObject)){
		zval_ptr_dtor(&queueObject);
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Get Rabbit Queue Object Fail ", 8 TSRMLS_CC);
		return;
	}

	//���ö��е�bind����
	MODULE_BEGIN
		zval	constructReturn,
				constructVal,
				*paramsList[2],
				params1,
				params2;
		paramsList[0] = &params1;
		paramsList[1] = &params2;
		MAKE_STD_ZVAL(paramsList[0]);
		MAKE_STD_ZVAL(paramsList[1]);
		ZVAL_STRING(paramsList[0],Z_STRVAL_P(exchangeZval),1);
		ZVAL_STRING(paramsList[1],Z_STRVAL_P(bindRouteZval),1);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"bind", 0);
		call_user_function(NULL, &queueObject, &constructVal, &constructReturn, 2, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_ptr_dtor(&paramsList[1]);
		zval_dtor(&constructReturn);
	MODULE_END

	//����ack����
	MODULE_BEGIN
		
		zval	constructReturn,
				constructVal,
				*paramsList[1],
				params1;
		paramsList[0] = &params1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_LONG(paramsList[0],tagId);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"ack", 0);
		call_user_function(NULL, &queueObject, &constructVal, &constructReturn,1, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);

		if(IS_LONG == Z_TYPE(constructReturn) && 1 == Z_LVAL(constructReturn)){
			ZVAL_TRUE(return_value);
		}else{
			ZVAL_FALSE(return_value);
		}
		zval_dtor(&constructReturn);
	MODULE_END


	zval_ptr_dtor(&queueObject);
}

PHP_METHOD(CRabbit,getQueue)
{
	char	*queue;
	int		queueLen = 0,
			tags = 0;

	zval	*queueObject;

	//��ȡ����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|l",&queue,&queueLen,&tags) == FAILURE){
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Call [CRabbit->getQueue] Parameter error ", 10001 TSRMLS_CC);
		return;
	}

	//��ȡ����
	CRabbit_getQueue(getThis(),queue,tags,&queueObject TSRMLS_CC);
	ZVAL_ZVAL(return_value,queueObject,1,1);
}

void CRabbit_getExchange(zval *object,char *name,char *exchangeType,int flags,int needDeclare,zval **returnObject TSRMLS_DC){

	zval	*exchangeListZval,
			**existObject;

	MAKE_STD_ZVAL(*returnObject);
	ZVAL_NULL(*returnObject);

	//��¼_exchangeName
	zend_update_property_string(CRabbitCe,object,ZEND_STRL("_exchangeName"),name TSRMLS_CC);

	//��ȡ�������б�
	exchangeListZval = zend_read_property(CRabbitCe,object,ZEND_STRL("_exchange"),0 TSRMLS_CC);
	if(IS_NULL == Z_TYPE_P(exchangeListZval)){
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		zend_update_property(CRabbitCe,object,ZEND_STRL("_exchange"),saveArray TSRMLS_CC);
		zval_ptr_dtor(&saveArray);
		exchangeListZval = zend_read_property(CRabbitCe,object,ZEND_STRL("_exchange"),0 TSRMLS_CC);
	}


	//�ж��Ƿ���ڴ˽�����
	if(0 == zend_hash_exists(Z_ARRVAL_P(exchangeListZval),name,strlen(name)+1)){

		//���������� AMQPExchange
		zval				*ampExchangeObject,
							*channelObjectZval;
		zend_class_entry	**exchangeClassPP,
							*exchangeClassCe;

		//��ȡchannelObject
		channelObjectZval = zend_read_property(CRabbitCe,object,ZEND_STRL("_channel"),0 TSRMLS_CC);

		//��ȡAMQPQueue����
		if(zend_hash_find(EG(class_table),"amqpexchange",strlen("amqpexchange")+1,(void**)&exchangeClassPP ) == FAILURE){
			zend_throw_exception(CQueueExceptionCe, "[CQueueException]The server is not currently installed AMQP extension", 1 TSRMLS_CC);
			return;
		}

		//ʵ��������
		exchangeClassCe = *exchangeClassPP;
		MAKE_STD_ZVAL(ampExchangeObject)
		object_init_ex(ampExchangeObject,exchangeClassCe);

		//���ù����� �������Ӷ���
		if (exchangeClassCe->constructor) {
			zval	constructReturn,
					constructVal,
					*paramsList[1],
					params1;

			paramsList[0] = &params1;
			MAKE_STD_ZVAL(paramsList[0]);
			ZVAL_ZVAL(paramsList[0],channelObjectZval,1,0);
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal, exchangeClassCe->constructor->common.function_name, 0);
			call_user_function(NULL, &ampExchangeObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
			zval_ptr_dtor(&paramsList[0]);
			zval_dtor(&constructReturn);
		}

		//����setName����
		MODULE_BEGIN
			zval	constructReturn,
					constructVal,
					*paramsList[1],
					params1;
			paramsList[0] = &params1;
			MAKE_STD_ZVAL(paramsList[0]);
			ZVAL_STRING(paramsList[0],name,1);
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal,"setName", 0);
			call_user_function(NULL, &ampExchangeObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
			zval_ptr_dtor(&paramsList[0]);
			zval_dtor(&constructReturn);
		MODULE_END

		//����setType
		MODULE_BEGIN
			zval	constructReturn,
					constructVal,
					*paramsList[1],
					params1;
			paramsList[0] = &params1;
			MAKE_STD_ZVAL(paramsList[0]);
			ZVAL_STRING(paramsList[0],exchangeType,1);
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal,"setType", 0);
			call_user_function(NULL, &ampExchangeObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
			zval_ptr_dtor(&paramsList[0]);
			zval_dtor(&constructReturn);
		MODULE_END

		//Tag��Ϊ0ʱ�����setTags
		if(0 != flags){
			zval	constructReturn,
					constructVal,
					*paramsList[1],
					params1;
			paramsList[0] = &params1;
			MAKE_STD_ZVAL(paramsList[0]);
			ZVAL_LONG(paramsList[0],flags);
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal,"setFlags", 0);
			call_user_function(NULL, &ampExchangeObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
			zval_ptr_dtor(&paramsList[0]);
			zval_dtor(&constructReturn);
		}

		//����declare
		if(1 == needDeclare){
			zval	constructReturn,
					constructVal;
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal,"declare", 0);
			call_user_function(NULL, &ampExchangeObject, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
			zval_dtor(&constructReturn);
		}


		//�洢
		add_assoc_zval(exchangeListZval,name,ampExchangeObject);
		zend_update_property(CRabbitCe,object,ZEND_STRL("_exchange"),exchangeListZval TSRMLS_CC);
		ZVAL_ZVAL(*returnObject,ampExchangeObject,1,0);
		return;
	}

	//ֱ�ӷ���
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(exchangeListZval),name,strlen(name)+1,(void**)&existObject) ){
		ZVAL_ZVAL(*returnObject,*existObject,1,0);
	}
}

PHP_METHOD(CRabbit,getExchange)
{

	char	*name = "default",
			*exchangeType = "direct";

	int		nameLen = 0,
			type = 0,
			exchangeTypeLen = 0,
			flags = 0;

	zval	*returnObject;

	//��ȡ����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|ssl",&name,&nameLen,&exchangeType,&exchangeTypeLen,&flags) == FAILURE){
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Call [CRabbit->getExchange] Parameter error ", 10001 TSRMLS_CC);
		return;
	}

	CRabbit_getExchange(getThis(),name,exchangeType,flags,1,&returnObject TSRMLS_CC);
	zval_ptr_dtor(&returnObject);
	ZVAL_ZVAL(return_value,getThis(),1,0);
}

PHP_METHOD(CRabbit,publish)
{

	char	*route,
			*message,
			*saveName;

	int		routeLen = 0,
			messageLen = 0;

	zval	*exchangeObject = NULL,
			*exchangeName;

	//��ȡ����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ss",&route,&routeLen,&message,&messageLen) == FAILURE){
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Call [CRabbit->publish] Parameter error , the message must be a string ", 10001 TSRMLS_CC);
		RETVAL_FALSE;
		return;
	}

	//��ȡ������
	exchangeName = zend_read_property(CRabbitCe,getThis(),ZEND_STRL("_exchangeName"),0 TSRMLS_CC);
	saveName = estrdup(Z_STRVAL_P(exchangeName));
	CRabbit_getExchange(getThis(),saveName,"direct",0,0,&exchangeObject TSRMLS_CC);

	//��ȡ������ʧ��
	if(IS_OBJECT != Z_TYPE_P(exchangeObject)){
		efree(saveName);
		zval_ptr_dtor(&exchangeObject);
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Call [CRabbit->getExchange] Fail , can not get an object ", 10004 TSRMLS_CC);
		RETVAL_FALSE;
		return;
	}

	//���ý�������publish����
	MODULE_BEGIN

		zval	constructReturn,
				constructVal,
				*paramsList[2],
				params1,
				params2;
		paramsList[0] = &params1;
		paramsList[1] = &params2;
		MAKE_STD_ZVAL(paramsList[0]);
		MAKE_STD_ZVAL(paramsList[1]);
		ZVAL_STRING(paramsList[0],message,1);
		ZVAL_STRING(paramsList[1],route,1);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"publish", 0);
		call_user_function(NULL, &exchangeObject, &constructVal, &constructReturn, 2, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_ptr_dtor(&paramsList[1]);

		if(IS_BOOL == Z_TYPE(constructReturn) && 1 == Z_LVAL(constructReturn)){
			ZVAL_BOOL(return_value,1);
		}else{
			ZVAL_BOOL(return_value,0);
		}

		zval_dtor(&constructReturn);

	MODULE_END


	zval_ptr_dtor(&exchangeObject);
	efree(saveName);
}

int CRabbit_llen(char *queue,zval *object TSRMLS_DC){

	zval				*channelObjectZval,
						*amqpQueueObject;
	int					message = 0;

	zend_class_entry	**amqpqueuePP,
						*amqpqueueCe;
	

	channelObjectZval = zend_read_property(CRabbitCe,object,ZEND_STRL("_channel"),0 TSRMLS_CC);
	if(IS_OBJECT != Z_TYPE_P(channelObjectZval)){
		return 0;
	}

	//����һ���¶���
	if(zend_hash_find(EG(class_table),"amqpqueue",strlen("amqpqueue")+1,(void**)&amqpqueuePP ) == FAILURE){
		zend_throw_exception(CQueueExceptionCe, "[CQueueException]The server is not currently installed AMQP extension", 1 TSRMLS_CC);
		return 0;
	}
	
	//��ȡ����
	amqpqueueCe = *amqpqueuePP;
	MAKE_STD_ZVAL(amqpQueueObject);
	object_init_ex(amqpQueueObject,amqpqueueCe);

	//���ù����� �������Ӷ���
	if (amqpqueueCe->constructor) {
		zval	constructReturn,
				constructVal,
				*paramsList[1],
				params1;

		paramsList[0] = &params1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_ZVAL(paramsList[0],channelObjectZval,1,0);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal, amqpqueueCe->constructor->common.function_name, 0);
		call_user_function(NULL, &amqpQueueObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_dtor(&constructReturn);
	}

	//����setName����
	MODULE_BEGIN
		zval	constructReturn,
				constructVal,
				*paramsList[1],
				params1;
		paramsList[0] = &params1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_STRING(paramsList[0],queue,1);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"setName", 0);
		call_user_function(NULL, &amqpQueueObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_dtor(&constructReturn);
	MODULE_END

	//Tag��Ϊ0ʱ�����setTags
	if(1){
		zval	constructReturn,
				constructVal,
				*paramsList[1],
				params1;
		paramsList[0] = &params1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_LONG(paramsList[0],4);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"setFlags", 0);
		call_user_function(NULL, &amqpQueueObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_dtor(&constructReturn);
	}

	//����declare
	MODULE_BEGIN
		zval	constructReturn,
				constructVal;
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"declare", 0);
		call_user_function(NULL, &amqpQueueObject, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
		if(IS_LONG == Z_TYPE(constructReturn)){
			message = Z_LVAL(constructReturn);
		}
		zval_dtor(&constructReturn);
	MODULE_END

	zval_ptr_dtor(&amqpQueueObject);

	return message;
}


PHP_METHOD(CRabbit,llen)
{

	char	*queue;
	int		queueLen = 0,
			messageCount = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&queue,&queueLen) == FAILURE){
		zend_throw_exception(CQueueExceptionCe, "[CQueueException] Call [CRabbit->llen] Parameter error , the queue must be a string ", 10001 TSRMLS_CC);
		return;
	}


	messageCount = CRabbit_llen(queue,getThis() TSRMLS_CC);
	RETVAL_LONG(messageCount);
}