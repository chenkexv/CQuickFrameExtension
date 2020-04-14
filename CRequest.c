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
#include "php_CRequest.h"
#include "php_CRouteParse.h"
#include "php_CRoute.h"
#include "php_CException.h"
#include "php_CWebApp.h"


//zend�෽��
zend_function_entry CRequset_functions[] = {
	PHP_ME(CRequest,getInstance,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,setController,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRequest,setAction,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRequest,setModule,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRequest,getController,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getAction,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getModule,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,__construct,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(CRequest,run,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRequest,_checkActionPreFix,NULL,ZEND_ACC_PRIVATE)
	PHP_ME(CRequest,routeDoing,NULL,ZEND_ACC_PRIVATE)
	PHP_ME(CRequest,createController,NULL,ZEND_ACC_PRIVATE)
	PHP_ME(CRequest,createAction,NULL,ZEND_ACC_PRIVATE)
	PHP_ME(CRequest,execAction,NULL,ZEND_ACC_PRIVATE)
	PHP_ME(CRequest,isSuccess,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CRequest,closeRouter,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,openRouter,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getUseRouterStatus,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,Args,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getUrl,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getUri,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getIp,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getPreUrl,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getAgent,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getHost,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getStartTime,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getRegisterEventTime,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getErrorMessage,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getPath,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,createUrl,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,disablePOST,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,disableGET,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,isWap,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,isCli,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,end,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,getRequestMethod,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CRequest,removeXSS,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};

//ģ�鱻����ʱ
CMYFRAME_REGISTER_CLASS_RUN(CRequset)
{
	zend_class_entry funCe;
	INIT_CLASS_ENTRY(funCe,"CRequest",CRequset_functions);
	CRequestCe = zend_register_internal_class(&funCe TSRMLS_CC);

	//���徲̬����
	zend_declare_property_null(CRequestCe, ZEND_STRL("instance"),ZEND_ACC_PRIVATE|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(CRequestCe, ZEND_STRL("controller"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CRequestCe, ZEND_STRL("action"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CRequestCe, ZEND_STRL("module"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CRequestCe, ZEND_STRL("errorMessage"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CRequestCe, ZEND_STRL("_useRouter"),ZEND_ACC_PRIVATE|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(CRequestCe, ZEND_STRL("controllerObj"),ZEND_ACC_PRIVATE TSRMLS_CC);

	return SUCCESS;
}

//��ȡCRequest��������
int CRequest_getInstance(zval **returnZval TSRMLS_DC)
{
	zval	*instanceZval;

	//��ȡ��̬instace����
	instanceZval = zend_read_static_property(CRequestCe,ZEND_STRL("instance"),0 TSRMLS_CC);

	//Ϊ��ʱ��ʵ��������
	if(IS_NULL == Z_TYPE_P(instanceZval) ){
		
		zval			*object,
						*saveObject;

		//ʵ�����ò��
		MAKE_STD_ZVAL(object);
		object_init_ex(object,CRequestCe);

		//ִ�й�����
		if (CRequestCe->constructor) {
			zval constructReturn;
			zval constructVal;
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal, CRequestCe->constructor->common.function_name, 0);
			call_user_function(NULL, &object, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
			zval_dtor(&constructReturn);
		}

		//������󱣴���instance��̬����
		zend_update_static_property(CRequestCe,ZEND_STRL("instance"),object TSRMLS_CC);

		//����
		MAKE_STD_ZVAL(*returnZval);
		ZVAL_ZVAL(*returnZval,object,1,1);
		return SUCCESS;
	}

	MAKE_STD_ZVAL(*returnZval);
	ZVAL_ZVAL(*returnZval,instanceZval,1,0);
	return SUCCESS;
}

//�෽��:����Ӧ�ö���
PHP_METHOD(CRequest,getInstance)
{
	zval *instanceZval;

	CRequest_getInstance(&instanceZval TSRMLS_CC);
	ZVAL_ZVAL(return_value,instanceZval,1,0);
	zval_ptr_dtor(&instanceZval);

}

PHP_METHOD(CRequest,getController)
{
	zval *controller;

	controller = zend_read_static_property(CRouteCe,ZEND_STRL("thisController"),0 TSRMLS_CC);
	RETVAL_STRING(Z_STRVAL_P(controller),1);
}

PHP_METHOD(CRequest,setController)
{
	char *val;
	int valLen;
	zval *saveZval;

	//���ÿ�����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&val,&valLen) == FAILURE){
		return;
	}

	//���±���
	MAKE_STD_ZVAL(saveZval);
	ZVAL_STRING(saveZval,val,1);
	zend_update_property(CRequestCe,getThis(),ZEND_STRL("controller"),saveZval TSRMLS_CC);
	zend_update_static_property_string(CRouteCe,ZEND_STRL("thisController"),val TSRMLS_CC);
	zval_ptr_dtor(&saveZval);
}

PHP_METHOD(CRequest,getAction)
{
	zval *action,
		 *cconfigInstanceZval,
		 *actionPrefix;


	action = zend_read_static_property(CRouteCe,ZEND_STRL("thisAction"),0 TSRMLS_CC);

	//��ȡ������ǰ׺
	CConfig_getInstance("main",&cconfigInstanceZval TSRMLS_CC);
	CConfig_load("ACTION_PREFIX",cconfigInstanceZval,&actionPrefix TSRMLS_CC);
	if(IS_STRING == Z_TYPE_P(actionPrefix)){

		char *replacePrefixAction;
		str_replace(Z_STRVAL_P(actionPrefix),"",Z_STRVAL_P(action),&replacePrefixAction);
		RETVAL_STRING(replacePrefixAction,1);
		efree(replacePrefixAction);

	}else{
		RETVAL_STRING(Z_STRVAL_P(action),1);
	}

	zval_ptr_dtor(&cconfigInstanceZval);
	zval_ptr_dtor(&actionPrefix);
}

PHP_METHOD(CRequest,setAction)
{
	char *val;
	int valLen;
	zval *saveZval;

	//���ÿ�����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&val,&valLen) == FAILURE){
		return;
	}

	//���±���
	MAKE_STD_ZVAL(saveZval);
	ZVAL_STRING(saveZval,val,1);
	zend_update_property(CRequestCe,getThis(),ZEND_STRL("action"),saveZval TSRMLS_CC);
	zend_update_static_property_string(CRouteCe,ZEND_STRL("thisAction"),val TSRMLS_CC);
	zval_ptr_dtor(&saveZval);
}

PHP_METHOD(CRequest,setModule)
{
	char *val;
	int valLen;
	zval *saveZval;

	//���ÿ�����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&val,&valLen) == FAILURE){
		return;
	}

	//���±���
	MAKE_STD_ZVAL(saveZval);
	ZVAL_STRING(saveZval,val,1);
	zend_update_property(CRequestCe,getThis(),ZEND_STRL("module"),saveZval TSRMLS_CC);
	zend_update_static_property_string(CRouteCe,ZEND_STRL("thisModule"),val TSRMLS_CC);
	zval_ptr_dtor(&saveZval);
}

PHP_METHOD(CRequest,getModule)
{
	zval *module;

	module = zend_read_static_property(CRouteCe,ZEND_STRL("thisModule"),0 TSRMLS_CC);
	RETVAL_STRING(Z_STRVAL_P(module),1);
}

PHP_METHOD(CRequest,__construct)
{
	zval	*cconfigInstanceZval,
			*thisConf,
			*saveZval;

	char	*defaultController,
			*defaultAction,
			*defaultModule;

	//���õ���
	CConfig_getInstance("main",&cconfigInstanceZval TSRMLS_CC);

	//������
	CConfig_load("DEFAULT_CONTROLLER",cconfigInstanceZval,&thisConf TSRMLS_CC);
	if(IS_STRING == Z_TYPE_P(thisConf)){
		defaultController = estrdup(Z_STRVAL_P(thisConf));
	}else{
		defaultController = estrdup("base");
	}

	zend_update_property_string(CRequestCe,getThis(),ZEND_STRL("controller"),defaultController TSRMLS_CC);
	zval_ptr_dtor(&thisConf);

	//����
	CConfig_load("DEFAULT_ACTION",cconfigInstanceZval,&thisConf TSRMLS_CC);
	if(IS_STRING == Z_TYPE_P(thisConf)){
		defaultAction = estrdup(Z_STRVAL_P(thisConf));
	}else{
		defaultAction = estrdup("index");
	}
	zend_update_property_string(CRequestCe,getThis(),ZEND_STRL("action"),defaultAction TSRMLS_CC);
	zval_ptr_dtor(&thisConf);

	//ģ��
	CConfig_load("DEFAULT_MODLUE",cconfigInstanceZval,&thisConf TSRMLS_CC);
	if(IS_STRING == Z_TYPE_P(thisConf)){
		defaultModule = estrdup(Z_STRVAL_P(thisConf));
	}else{
		defaultModule = estrdup("www");
	}

	zend_update_property_string(CRequestCe,getThis(),ZEND_STRL("module"),defaultModule TSRMLS_CC);
	zval_ptr_dtor(&thisConf);

	zend_update_static_property_string(CRouteCe,ZEND_STRL("thisController"),defaultController TSRMLS_CC);
	zend_update_static_property_string(CRouteCe,ZEND_STRL("thisAction"),defaultAction TSRMLS_CC);
	zend_update_static_property_string(CRouteCe,ZEND_STRL("thisModule"),defaultModule TSRMLS_CC);

	efree(defaultController);
	efree(defaultAction);
	efree(defaultModule);

	zval_ptr_dtor(&cconfigInstanceZval);
}

int checkChildClass(zend_class_entry *controllerEntryP)
{
	if(!controllerEntryP->parent){
		return FAILURE;
	}else if(controllerEntryP->parent && strcmp("CController",controllerEntryP->parent->name) != 0){
		return checkChildClass(controllerEntryP->parent);
	}else if(controllerEntryP->parent && strcmp("CController",controllerEntryP->parent->name) == 0){
		return SUCCESS;
	}
}


//����������
void CRequest_createController(char *path,char *name,zval **returnZval TSRMLS_DC)
{
	zval	*controllerObject,
			*saveController;

	zend_class_entry	**controllerCePP,
						*controllerCe;

	char *tureClassName;

	MAKE_STD_ZVAL(*returnZval);
	ZVAL_NULL(*returnZval);

	tureClassName = estrdup(name);
	php_strtolower(tureClassName,strlen(tureClassName)+1);


	//������ļ�
	if(zend_hash_find(EG(class_table),tureClassName,strlen(tureClassName)+1,(void**)&controllerCePP ) == FAILURE){
		if(CLoader_loadFile(path) != SUCCESS){
			char *errMessage;
			strcat2(&errMessage,"[CClassNotFoundException] Cannot load controller files[",name,"]",NULL);
			zend_throw_exception(CClassNotFoundExceptionCe, errMessage, 100001 TSRMLS_CC);
			efree(errMessage);
			return;
		}
	}

	//��ѯ����ṹ��
	if(zend_hash_find(EG(class_table),tureClassName,strlen(tureClassName)+1,(void**)&controllerCePP ) == FAILURE){
		char *errMessage;
		strcat2(&errMessage,"[RouteException] Can't find the controller files[",name,"],Confirm the identity of the controller file name and the name of the class",NULL);
		php_error_docref(NULL TSRMLS_CC, E_ERROR,"%s",errMessage);
		return;
	}
	efree(tureClassName);

	//ȡ��ַ
	controllerCe = *controllerCePP;
	
	//�Ƿ�Ϊ������
	if(controllerCe->ce_flags & ZEND_ACC_EXPLICIT_ABSTRACT_CLASS){
		char *errMessage;
		strcat2(&errMessage,"[RouteException] The controller",name,"] Cannot be designed into an abstract class",NULL);
		zend_throw_exception(CRouteExceptionCe, errMessage, 100002 TSRMLS_CC);
		efree(errMessage);
		return;
	}

	//�Ƿ�Ϊ�ӿ�
	if(controllerCe->ce_flags & ZEND_ACC_INTERFACE){
		char *errMessage;
		strcat2(&errMessage,"[RouteException] The controller",name,"] Can't be designed interface",NULL);
		zend_throw_exception(CRouteExceptionCe, errMessage, 100002 TSRMLS_CC);
		efree(errMessage);
		return;
	}

	//����Ƿ�̳�CController
	if(FAILURE ==  checkChildClass(controllerCe)){
		char *errMessage;
		strcat2(&errMessage,"[RouteException] The controller[",name,"] Should inherit CController main controller",NULL);
		zend_throw_exception(CRouteExceptionCe, errMessage, 100002 TSRMLS_CC);
		efree(errMessage);
		return;
	}

	//ʵ����
	MAKE_STD_ZVAL(controllerObject);
    object_init_ex(controllerObject,controllerCe);

	//�����乹�캯��
	if (controllerCe->constructor) {
		zval constructReturn;
		zval constructVal,
			 *diInstanceZval,
			 *paramsList[1],
			 param1;
		INIT_ZVAL(constructVal);
		CDiContainer_getInstance(&diInstanceZval TSRMLS_CC);
		paramsList[0] = &param1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_ZVAL(paramsList[0],diInstanceZval,1,1);
		ZVAL_STRING(&constructVal, controllerCe->constructor->common.function_name, 0);
		call_user_function(NULL, &controllerObject, &constructVal, &constructReturn, 1, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_dtor(&constructReturn);
	}

	//��������������뾲̬����
	zend_update_static_property(CRouteCe,ZEND_STRL("controllerObject"),controllerObject TSRMLS_CC);

	//���ؿ���������
	ZVAL_ZVAL(*returnZval,controllerObject,1,1);
	return;
}

//����������
PHP_METHOD(CRequest,createController)
{
	char	*path,
			*name;

	int		pathLen,
			nameLen;

	zval	*returnZval;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"ss",&path,&pathLen,&name,&nameLen) == FAILURE){
		RETVAL_NULL();
		return;
	}

	//���ô�������������
	CRequest_createController(path,name,&returnZval TSRMLS_CC);

	//����������
	zend_update_property(CRequestCe,getThis(),ZEND_STRL("controllerObj")+1,returnZval TSRMLS_CC);
	ZVAL_ZVAL(return_value,returnZval,1,0);
	zval_ptr_dtor(&returnZval);
}


//����·�ɶ���
void CRequest_checkActionPreFix(zval *route,zval **returnZval TSRMLS_DC)
{
	zval	*cconfigInstanceZval,
			*actionPreZval,
			*routeInstance,
			*thisController,
			*thisAction,
			*thisModule,
			*setStatus;

	char	*controller,
			*action,
			*module;

	//���õ���
	CConfig_getInstance("main",&cconfigInstanceZval TSRMLS_CC);

	//����ǰ׺
	CConfig_load("ACTION_PREFIX",cconfigInstanceZval,&actionPreZval TSRMLS_CC);

	zval_ptr_dtor(&cconfigInstanceZval);
	
	thisController = zend_read_static_property(CRouteCe,ZEND_STRL("thisController"),0 TSRMLS_CC);
	thisAction = zend_read_static_property(CRouteCe,ZEND_STRL("thisAction"),0 TSRMLS_CC);
	thisModule = zend_read_static_property(CRouteCe,ZEND_STRL("thisModule"),0 TSRMLS_CC);
	
	//��ֵ�ӽṹ����ȡ��
	controller = estrdup(Z_STRVAL_P(thisController));
	action = estrdup(Z_STRVAL_P(thisAction));
	module = estrdup(Z_STRVAL_P(thisModule));

	//��鷽����ǰ׺
	if(IS_STRING == Z_TYPE_P(actionPreZval) && strlen(Z_STRVAL_P(actionPreZval)) >0 ){
		efree(action);
		strcat2(&action,Z_STRVAL_P(actionPreZval),Z_STRVAL_P(thisAction),NULL);
	}

	//��ȡ·�ɵ�������
	CRoute_getInstance(&routeInstance TSRMLS_CC);

	//����·�ɽ��
	CRoute_setController((controller),routeInstance,&setStatus TSRMLS_CC);
	CRoute_setAction((action),routeInstance,&setStatus TSRMLS_CC);
	CRoute_setModule((module),routeInstance,&setStatus TSRMLS_CC);

	zval_ptr_dtor(&actionPreZval);

	efree(controller);
	efree(action);
	efree(module);

	MAKE_STD_ZVAL(*returnZval);
	ZVAL_ZVAL(*returnZval,routeInstance,1,1);
	return;
}

//�������󷽷�
void CRequest_createAction(zval *controllerObject,char *requsetAction TSRMLS_DC)
{
	zval	fnReturn,
			fnName;

	//ִ�з���
	INIT_ZVAL(fnName);
	ZVAL_STRING(&fnName,requsetAction,0);
	call_user_function(NULL, &controllerObject, &fnName, &fnReturn, 0, NULL TSRMLS_CC);
	zval_dtor(&fnReturn);
}

//�������󷽷�
PHP_METHOD(CRequest,createAction)
{
	zval	*controllerObject;

	char	*actionName;
	int		actionNameLen;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"zs",&controllerObject,&actionName,&actionNameLen) == FAILURE){
		RETVAL_NULL();
		return;
	}

	CRequest_createAction(controllerObject,actionName TSRMLS_CC);

}

//����·�ɶ���
PHP_METHOD(CRequest,_checkActionPreFix)
{
	zval	*returnZval,
			*routeZval;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&routeZval) == FAILURE){
		RETVAL_NULL();
		return;
	}

	CRequest_checkActionPreFix(routeZval,&returnZval TSRMLS_CC);
	ZVAL_ZVAL(return_value,returnZval,1,1);
}

//ִ��·��
void CRequest_routeDoing(zval *routeObject TSRMLS_DC)
{
	zval	*returnZval,
			*cconfigInstanceZval,
			*defaultModelZval,
			*useModule,
			*codePath,
			*appPath,
			*module,
			*controller;

	char	*requsetModule,
			*requestController;

	//���õ���
	CConfig_getInstance("main",&cconfigInstanceZval TSRMLS_CC);

	//���Ĭ��ģ��
	CConfig_load("DEFAULT_MODLUE",cconfigInstanceZval,&defaultModelZval TSRMLS_CC);
	if(IS_NULL == Z_TYPE_P(defaultModelZval)){
		ZVAL_STRING(defaultModelZval,"www",1);
	}

	//CODEPAHT
	codePath = zend_read_static_property(CWebAppCe,ZEND_STRL("code_path"),0 TSRMLS_CC);
	appPath = zend_read_static_property(CWebAppCe,ZEND_STRL("app_path"),0 TSRMLS_CC);
	module = zend_read_static_property(CRouteCe,ZEND_STRL("thisModule"),0 TSRMLS_CC);
	controller = zend_read_static_property(CRouteCe,ZEND_STRL("thisController"),0 TSRMLS_CC);

	//��ǰģ��
	requsetModule = estrdup(Z_STRVAL_P(module));
	requestController = estrdup(Z_STRVAL_P(controller));


	//ʵ����������
	if(NULL == requestController){
		zval_ptr_dtor(&cconfigInstanceZval);
		zval_ptr_dtor(&defaultModelZval);
		php_error_docref(NULL TSRMLS_CC, E_ERROR,"[RouteException]The request of the path, the file does not exist or access");
		return;
	}

	//ȷ���Ƿ�ʹ��ģ��
	CConfig_load("USE_MODULE",cconfigInstanceZval,&useModule TSRMLS_CC);
	if(IS_NULL == Z_TYPE_P(useModule)){
		ZVAL_BOOL(useModule,0);
	}

	//��ģ��
	if( (IS_BOOL == Z_TYPE_P(useModule) && Z_LVAL_P(useModule) == 0) || strcmp(Z_STRVAL_P(defaultModelZval),requsetModule) == 0 ){

		char	*controllerPath,
				*controllLessName;

		zval *controllerMapConfItem;

		zend_class_entry	**controllerCePP;

		int hasExistController = FAILURE;

		//�������ļ�
		strcat2(&controllerPath,Z_STRVAL_P(codePath),"/controllers/",requestController,".php",NULL);

		//check class exists
		controllLessName = estrdup(requestController);
		php_strtolower(controllLessName,strlen(controllLessName)+1);
		hasExistController = zend_hash_find(EG(class_table),controllLessName,strlen(controllLessName)+1,(void**)&controllerCePP);
		efree(controllLessName);


		if(SUCCESS == fileExist(controllerPath) || SUCCESS == hasExistController){
			//��������������
			zval *getReturn;
			CRequest_createController(controllerPath,(requestController),&getReturn TSRMLS_CC);
			zval_ptr_dtor(&getReturn);
			efree(controllerPath);
		}else{
			char *errMessage;
			strcat2(&errMessage,"[RouteException] Has not been defined controller:",requestController,NULL);
			efree(controllerPath);
			efree(requsetModule);
			efree(requestController);
			zval_ptr_dtor(&useModule);
			zval_ptr_dtor(&cconfigInstanceZval);
			zval_ptr_dtor(&defaultModelZval);
			CHooks_callHooks("HOOKS_ROUTE_ERROR",NULL,0 TSRMLS_CC);
			zend_throw_exception(CRouteExceptionCe, errMessage, 100000 TSRMLS_CC);	
			efree(errMessage);
			return;
		}
	}else{

		//����ģ��
		char	*moduleName,
				*modulePath;

		moduleName = estrdup(Z_STRVAL_P(module));
		strcat2(&modulePath,Z_STRVAL_P(appPath),"/modules/",moduleName,NULL);

		//�ж��Ƿ���ڸ�ģ��Ŀ¼
		if(SUCCESS == fileExist(modulePath)){
		
			//���������ļ�
			char *moduleControllerPath;
			strcat2(&moduleControllerPath,Z_STRVAL_P(appPath),"/modules/",moduleName,"/controllers/",requestController,".php",NULL);
			if(SUCCESS == fileExist(moduleControllerPath)){
				//��������������
				zval *getReturn;
				CRequest_createController(moduleControllerPath,requestController,&getReturn TSRMLS_CC);
				zval_ptr_dtor(&getReturn);
				efree(moduleName);
				efree(modulePath);
				efree(moduleControllerPath);
			}else{
				char *errMessage;
				strcat2(&errMessage,"[RouteException] Module[",moduleName,"] Has not been defined controller:",requestController,NULL);
				efree(moduleName);
				efree(modulePath);
				efree(requsetModule);
				efree(requestController);
				efree(moduleControllerPath);
				zval_ptr_dtor(&cconfigInstanceZval);
				CHooks_callHooks("HOOKS_ROUTE_ERROR",NULL,0 TSRMLS_CC);	
				zend_throw_exception(CRouteExceptionCe, errMessage, 100000 TSRMLS_CC);
				efree(errMessage);
				return;
			}
		}else{
			//��������
			char *errMessage;
			strcat2(&errMessage,"[RouteException]The module requested does not exist:",moduleName,NULL);
			efree(moduleName);
			efree(requsetModule);
			efree(requestController);
			zval_ptr_dtor(&cconfigInstanceZval);
			php_error_docref(NULL TSRMLS_CC, E_ERROR,"%s",errMessage);
			return;
		}
	}

	zval_ptr_dtor(&useModule);
	zval_ptr_dtor(&defaultModelZval);
	efree(requsetModule);
	efree(requestController);
	zval_ptr_dtor(&cconfigInstanceZval);
}

//ִ��·��
PHP_METHOD(CRequest,routeDoing)
{
	zval	*routeObject;


	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&routeObject) == FAILURE){
		RETVAL_NULL();
		return;
	}
	CRequest_routeDoing(routeObject TSRMLS_CC);
	RETVAL_NULL();

}

//ִ�з���
void CRequest_execAction(zval *routeObject,zval *object TSRMLS_DC)
{
	zval	*controllerObject,
			*trueAction,
			*actionPreZval,
			*thisController,
			*thisAction;

	char	*requsetController,
			*requsetAction;


	zend_class_entry	**controllerCePP,
						*controllerCe;

	zend_function		*requsetActionEntry;

	thisController = zend_read_static_property(CRouteCe,ZEND_STRL("thisController"),0 TSRMLS_CC);
	thisAction = zend_read_static_property(CRouteCe,ZEND_STRL("thisAction"),0 TSRMLS_CC);
	controllerObject = zend_read_static_property(CRouteCe,ZEND_STRL("controllerObject"),0 TSRMLS_CC);


	//�������
	requsetController = estrdup(Z_STRVAL_P(thisController));
	requsetAction = estrdup(Z_STRVAL_P(thisAction));

	//����������ʧ
	if(IS_OBJECT != Z_TYPE_P(controllerObject)){
		efree(requsetController);
		efree(requsetAction);
		zend_throw_exception(CExceptionCe, "[CQuickFrameworkFatal] CQuickFramework fatal error:CMyRoute structure reference object is missing", 100000 TSRMLS_CC);	
		return;
	}

	//����ʽ������
	if(strlen(requsetAction) <= 0){
		efree(requsetController);
		efree(requsetAction);
		zend_throw_exception(CRouteExceptionCe, "[RouteException]The system can't find the request method", 100002 TSRMLS_CC);	
		return;
	}

	//��ȡ��ṹ��
	php_strtolower(requsetController,strlen(requsetController)+1);
	zend_hash_find(EG(class_table),requsetController,strlen(requsetController)+1,(void**)&controllerCePP);
	controllerCe = *controllerCePP;

	//������__beforeħ������
	if(zend_hash_exists(&controllerCe->function_table,"__before",strlen("__before")+1)){

		zend_function    *beforeEntry;
		//�жϺ����Ƿ�ɵ�
		zend_hash_find(&controllerCe->function_table,"__before",strlen("__before")+1,(void **)&beforeEntry);
		if(beforeEntry->common.fn_flags & ZEND_ACC_PUBLIC){
			//����before����
			zval fnReturn;
			zval fnName;
			INIT_ZVAL(fnName);
			ZVAL_STRING(&fnName,"__before", 0);
			call_user_function(NULL, &controllerObject, &fnName, &fnReturn, 0, NULL TSRMLS_CC);
			zval_dtor(&fnReturn);
		}else{
			php_error_docref(NULL TSRMLS_CC, E_WARNING ,"[RuntimeWarnning]The controller[%s] Define a magic methods __before but access is not enough to provide CQuickFramework calls",requsetController);
		}
	}

	//set language
	MODULE_BEGIN
		zval	constructReturn,
				constructVal;
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal, "setLanguage", 0);
		call_user_function(NULL, &controllerObject, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
		zval_dtor(&constructReturn);
	MODULE_END


	//������__dispatchħ������
	if(zend_hash_exists(&controllerCe->function_table,"__dispatch",strlen("__dispatch")+1)){

		zend_function    *beforeEntry;
		//�жϺ����Ƿ�ɵ�
		zend_hash_find(&controllerCe->function_table,"__dispatch",strlen("__dispatch")+1,(void **)&beforeEntry);
		if(beforeEntry->common.fn_flags & ZEND_ACC_PUBLIC){
			//����before����
			zval fnReturn;
			zval fnName;
			zval *params[1];
			MAKE_STD_ZVAL(params[0]);
			ZVAL_STRING(params[0],requsetAction,1);
			INIT_ZVAL(fnName);
			ZVAL_STRING(&fnName,"__dispatch", 0);
			call_user_function(NULL, &controllerObject, &fnName, &fnReturn, 1, params TSRMLS_CC);
			zval_dtor(&fnReturn);
			efree(requsetController);
			efree(requsetAction);
			zval_ptr_dtor(&params[0]);
			return;
		}
	}



	//�����������󷽷�
	php_strtolower(requsetAction,strlen(requsetAction)+1);
	if(!zend_hash_exists(&controllerCe->function_table,requsetAction,strlen(requsetAction)+1)){
	
		//�����������󷽷�ʱ ���Լ��__errorħ������
		if(zend_hash_exists(&controllerCe->function_table,"__error",strlen("__error")+1)){
			zend_function    *errorEntry;
			//�жϺ����Ƿ�ɵ�
			zend_hash_find(&controllerCe->function_table,"__error",strlen("__error")+1,(void **)&errorEntry);
			if(errorEntry->common.fn_flags & ZEND_ACC_PUBLIC){
				//����error����
				zval fnReturn;
				zval fnName;
				INIT_ZVAL(fnName);
				ZVAL_STRING(&fnName,"__error", 0);
				call_user_function(NULL, &controllerObject, &fnName, &fnReturn, 0, NULL TSRMLS_CC);
				zval_dtor(&fnReturn);
				efree(requsetController);
				efree(requsetAction);

				return;
			}else{
				php_error_docref(NULL TSRMLS_CC, E_WARNING ,"[RuntimeWarnning]The controller[%s] Define a magic methods __error but access is not enough to provide CQuickFramework calls",estrdup(requsetController));
				return;
			}
		}else{
			char *errMessage;
			strcat2(&errMessage,"[RouteException]The requested method does not exist:",requsetAction,NULL);
			CHooks_callHooks("HOOKS_ROUTE_ERROR",NULL,0 TSRMLS_CC);
			zend_throw_exception(CRouteExceptionCe, errMessage, 100002 TSRMLS_CC);
			efree(errMessage);
			efree(requsetController);
			efree(requsetAction);
			return;
		}
	}

	//�ж����󷽷��ķ���Ȩ��
	zend_hash_find(&controllerCe->function_table,requsetAction,strlen(requsetAction)+1,(void **)&requsetActionEntry);
	if(!requsetActionEntry->common.fn_flags & ZEND_ACC_PUBLIC){
		char *errMessage;
		strcat2(&errMessage,"[RouteException] Action [",requsetAction,"] Don't have access",NULL);
		zend_throw_exception(CRouteExceptionCe, errMessage, 100002 TSRMLS_CC);
		efree(errMessage);
		efree(requsetController);
		efree(requsetAction);
		return;
	}

	//ִ�����󷽷�
	CRequest_createAction(controllerObject,requsetAction TSRMLS_CC);

	efree(requsetController);
	efree(requsetAction);
}

//ִ�з���
PHP_METHOD(CRequest,execAction)
{
	zval	*routeObject;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&routeObject) == FAILURE){
		RETVAL_NULL();
		return;
	}
	CRequest_execAction(routeObject,getThis() TSRMLS_CC);


	RETVAL_NULL();
}

PHP_METHOD(CRequest,isSuccess)
{
	RETVAL_TRUE;
}

PHP_METHOD(CRequest,closeRouter)
{
	//���¹ر�·��
	zend_update_static_property_bool(CRequestCe,ZEND_STRL("_useRouter"),0 TSRMLS_CC);
	RETVAL_TRUE;
}

PHP_METHOD(CRequest,openRouter)
{
	//����·��
	zend_update_static_property_bool(CRequestCe,ZEND_STRL("_useRouter"),1 TSRMLS_CC);
	RETVAL_TRUE;
}

PHP_METHOD(CRequest,getUseRouterStatus)
{
	zval *useRouter;
	useRouter = zend_read_static_property(CRequestCe,ZEND_STRL("_useRouter"),0 TSRMLS_CC);

	RETVAL_BOOL(Z_LVAL_P(useRouter));
}

void CRequest_filterHTML(zval *array,zval **newArray TSRMLS_DC)
{
	int		i,h;
	zval	**thisVal;
	char	*filterString,
			*otherKey;
	ulong	thisIntKey;

	if(IS_ARRAY != Z_TYPE_P(array)){
		return;
	}

	MAKE_STD_ZVAL(*newArray);
	array_init(*newArray);

	zend_hash_internal_pointer_reset(Z_ARRVAL_P(array));
	h = zend_hash_num_elements(Z_ARRVAL_P(array));
	for(i = 0 ; i < h;i++){
		zend_hash_get_current_data(Z_ARRVAL_P(array),(void**)&thisVal);

		if(IS_STRING != Z_TYPE_PP(thisVal)){
			zval_ptr_dtor(newArray);
			MAKE_STD_ZVAL(*newArray);
			ZVAL_ZVAL(*newArray,array,1,0);
			return;
		}

		convert_to_string(*thisVal);
		strip_tags(Z_STRVAL_PP(thisVal),&filterString);

		if(HASH_KEY_IS_LONG == zend_hash_get_current_key_type(Z_ARRVAL_P(array))){
			zend_hash_get_current_key(Z_ARRVAL_P(array), &otherKey, &thisIntKey, 0);
			add_index_string(*newArray,thisIntKey,filterString,1);
		}else if(HASH_KEY_IS_STRING == zend_hash_get_current_key_type(Z_ARRVAL_P(array))){
			zend_hash_get_current_key(Z_ARRVAL_P(array), &otherKey, &thisIntKey, 0);
			add_assoc_string(*newArray,otherKey,filterString,1);
		}
		efree(filterString);
		zend_hash_move_forward(Z_ARRVAL_P(array));
	}
}

void CRequest_string_htmlspecialchars(char *replaceTempString1,char **replaceTempString2 TSRMLS_DC)
{
	char	*tempString1,
			*tempString2;

	zval	*find,
			*replace;

	MAKE_STD_ZVAL(find);
	MAKE_STD_ZVAL(replace);
	array_init(find);
	array_init(replace);
	
	add_next_index_string(find,"&",1);
	add_next_index_string(find,"\"",1);
	add_next_index_string(find,"<",1);
	add_next_index_string(find,">",1);

	add_next_index_string(replace,"&amp;",1);
	add_next_index_string(replace,"&quot;",1);
	add_next_index_string(replace,"&lt;",1);
	add_next_index_string(replace,"&gt;",1);

	str_replaceArray(find,replace,replaceTempString1,&tempString1);

	if(strstr(tempString1,"&amp;#") != NULL){
		preg_replace("/&amp;((#(\\d{3,5}|x[a-fA-F0-9]{4}));)/", "&\\1", tempString1,&tempString2);
		*replaceTempString2 = estrdup(tempString2);
		efree(tempString2);
	}else{
		*replaceTempString2 = estrdup(replaceTempString1);
	}

	zval_ptr_dtor(&replace);
	zval_ptr_dtor(&find);
	efree(tempString1);

}

void CRequest_xssRemove(char *string,char **result TSRMLS_DC){

	zval	*match,
			**match1,
			*findTags,
			**thisVal,
			*tempZval1,
			*searchs,
			*replaces;

	char	*allowtags = "img|a|font|div|table|tbody|caption|tr|td|th|br|p|b|strong|i|u|em|span|ol|ul|li|blockquote",
			*skipkeysString = "/(onabort|onactivate|onafterprint|onafterupdate|onbeforeactivate|onbeforecopy|onbeforecut|onbeforedeactivate|onbeforeeditfocus|onbeforepaste|onbeforeprint|onbeforeunload|onbeforeupdate|onblur|onbounce|oncellchange|onchange|onclick|oncontextmenu|oncontrolselect|oncopy|oncut|ondataavailable|ondatasetchanged|ondatasetcomplete|ondblclick|ondeactivate|ondrag|ondragend|ondragenter|ondragleave|ondragover|ondragstart|ondrop|onerror|onerrorupdate|onfilterchange|onfinish|onfocus|onfocusin|onfocusout|onhelp|onkeydown|onkeypress|onkeyup|onlayoutcomplete|onload|onlosecapture|onmousedown|onmouseenter|onmouseleave|onmousemove|onmouseout|onmouseover|onmouseup|onmousewheel|onmove|onmoveend|onmovestart|onpaste|onpropertychange|onreadystatechange|onreset|onresize|onresizeend|onresizestart|onrowenter|onrowexit|onrowsdelete|onrowsinserted|onscroll|onselect|onselectionchange|onselectstart|onstart|onstop|onsubmit|onunload|javascript|script|eval|behaviour|expression|style|class)/i",
			*replaceTempString1,
			*replaceTempString2,
			*replaceTempString3,
			*replaceTempString4,
			*replaceTempString5,
			*replaceTempString6,
			*replaceTempString9,
			*replaceTempString10,
			*allowTags = "/^[\\/|\\s]?(img|a|font|div|table|tbody|caption|tr|td|th|br|p|b|strong|i|u|em|span|ol|ul|li|blockquote)(\\s+|$)/is",
			*searchTagsTemp;

	int		i,h;

	if(!preg_match_all("/\\<([^\\<]+)\\>/is", string, &match)){
		*result = estrdup(string);
		zval_ptr_dtor(&match);
		return;
	}

	if(IS_ARRAY != Z_TYPE_P(match)){
		*result = estrdup(string);
		zval_ptr_dtor(&match);
		return;
	}

	//read1
	if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(match),1,(void**)&match1) && IS_ARRAY == Z_TYPE_PP(match1)){
	}else{
		*result = estrdup(string);
		zval_ptr_dtor(&match);
		return;
	}

	MAKE_STD_ZVAL(searchs);
	MAKE_STD_ZVAL(replaces);
	array_init(searchs);
	array_init(replaces);

	add_next_index_string(searchs,"<",1);
	add_next_index_string(searchs,">",1);
	add_next_index_string(replaces,"&lt;",1);
	add_next_index_string(replaces,"&gt;",1);
		
	//qu chong
	array_unique(*match1,&findTags);

	//foreach
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(findTags));
	h = zend_hash_num_elements(Z_ARRVAL_P(findTags));
	for(i = 0 ; i < h ; i++){
		zend_hash_get_current_data(Z_ARRVAL_P(findTags),(void**)&thisVal);

		spprintf(&searchTagsTemp,0,"%s%s%s","&lt;",Z_STRVAL_PP(thisVal),"&gt;");
		add_next_index_string(searchs,searchTagsTemp,1);
		efree(searchTagsTemp);

		str_replace("'&amp;","_uch_tmp_str_",Z_STRVAL_PP(thisVal),&replaceTempString1);
		CRequest_string_htmlspecialchars(replaceTempString1,&replaceTempString2 TSRMLS_CC);
		str_replace("_uch_tmp_str_","'&amp;",replaceTempString2,&replaceTempString3);
		str_replace("\\",".",replaceTempString3,&replaceTempString4);
		str_replace("/*","/.",replaceTempString4,&replaceTempString5);
		preg_replace(skipkeysString,".",replaceTempString5,&replaceTempString6);
		
		if(!preg_match(allowTags,replaceTempString6,&tempZval1)){
			efree(replaceTempString6);
			replaceTempString6 = estrdup("");
		}

		//relace and append
		if(strlen(replaceTempString6) <= 0){
			replaceTempString9 = estrdup("");
		}else{
			char	*replaceTempString7;
			str_replace("&quot;","\"",replaceTempString6,&replaceTempString7);
			spprintf(&replaceTempString9,0,"%s%s%s","<",replaceTempString7,">");
			efree(replaceTempString7);
		}

		add_next_index_string(replaces,replaceTempString9,1);

		efree(replaceTempString1);
		efree(replaceTempString2);
		efree(replaceTempString3);
		efree(replaceTempString4);
		efree(replaceTempString5);
		efree(replaceTempString6);
		efree(replaceTempString9);
		zval_ptr_dtor(&tempZval1);

		zend_hash_move_forward(Z_ARRVAL_P(findTags));
	}

	str_replaceArray(searchs,replaces,string,&replaceTempString10);

	*result = estrdup(replaceTempString10);

	//destoy
	zval_ptr_dtor(&findTags);
	zval_ptr_dtor(&match);
	efree(replaceTempString10);
	zval_ptr_dtor(&replaces);
	zval_ptr_dtor(&searchs);
}

PHP_METHOD(CRequest,removeXSS)
{
	char	*string,
			*returnString;
	int		stringLen = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&string,&stringLen) == FAILURE){
		RETVAL_FALSE;
		return;
	}

	CRequest_xssRemove(string,&returnString TSRMLS_CC);
	RETVAL_STRING(returnString,1);
	efree(returnString);
}


void CRequest_Args(char *key,char *type,char *from,int noFilter,zval **returnString TSRMLS_DC){

	char *getReturn,
		 *lowFrom,
		 *lowType;
	MAKE_STD_ZVAL(*returnString);
	if(key == "" || key == NULL){
		ZVAL_NULL(*returnString);
		return;
	}

	lowType = estrdup(type);
	php_strtolower(lowType,strlen(lowType)+1);


	//ֱ��ȡPOST�е�array
	if(strcmp(lowType,"array") == 0){
		zval *postArray = NULL;
		getPostParamsZval(key,&postArray);
		if(IS_NULL != Z_TYPE_P(postArray)){
			zval	*filterArray;
			//fiter html
			CRequest_filterHTML(postArray,&filterArray TSRMLS_CC);
			ZVAL_ZVAL(*returnString,filterArray,1,1);
			zval_ptr_dtor(&postArray);
			efree(lowType);
			return;
		}else{
			zval_ptr_dtor(&postArray);
			array_init(*returnString);
			efree(lowType);
			return;
		}
	}

	//��fromͳһ��Сд
	lowFrom = estrdup(from);
	php_strtolower(lowFrom,strlen(lowFrom)+1);
	
	if(strcmp(lowFrom,"get") == 0){

		getGetParams(key,&getReturn);
		if(getReturn == NULL){
			getReturn = estrdup("");
		}

	}else if(strcmp(lowFrom,"post") == 0){

		getPostParams(key,&getReturn);
		if(getReturn == NULL){
			getReturn = estrdup("");
		}

	}else{

		//δָ�� ����ȡPOST ��������ȡGET
		getPostParams(key,&getReturn);
		if(getReturn == NULL){
			getGetParams(key,&getReturn);
			if(getReturn == NULL){
				getReturn = estrdup("");
			}
		}
	}
	efree(lowFrom);


	//�ж�����
	if(strcmp(lowType,"int") == 0){
		int thisVal = toInt(getReturn);
		ZVAL_LONG(*returnString,thisVal);
	}else if(strcmp(lowType,"string") == 0){

		if(noFilter == 1){
			ZVAL_STRING(*returnString,getReturn,1);
		}else{
			char	*thisVal,
					*xssFilterString;
			strip_tags(getReturn,&thisVal);
			ZVAL_STRING(*returnString,thisVal,1);
			efree(thisVal);
		}
	}else if(strcmp(lowType,"html") == 0){
		//filter xss
		char	*xssFilterString;
		CRequest_xssRemove(getReturn,&xssFilterString TSRMLS_CC);
		ZVAL_STRING(*returnString,xssFilterString,1);
		efree(xssFilterString);
	}else{
		if(noFilter == 1){
			ZVAL_STRING(*returnString,getReturn,1);
		}else{
			char *thisVal;
			CRequest_xssRemove(getReturn,&thisVal TSRMLS_CC);
			ZVAL_STRING(*returnString,thisVal,1);
			efree(thisVal);
		}
	}

	efree(lowType);
	efree(getReturn);
}

PHP_METHOD(CRequest,Args)
{
	
	char *key = "",
		 *type = "string",
		 *from = "REQUEST";

	zval *returnString;

	int keyLen = 0,
		typeLen = 0,
		fromLen = 0,
		noFilter = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|ssb",&key,&keyLen,&type,&typeLen,&from,&fromLen,&noFilter) == FAILURE){
		RETURN_NULL();
	}

	CRequest_Args(key,type,from,noFilter,&returnString TSRMLS_CC);
	RETVAL_ZVAL(returnString,1,1);
}

PHP_METHOD(CRequest,getUrl)
{
	zval *requsetUri;

	char	*host,
			*url,
			*filter;
	getServerParam("HTTP_HOST",&host TSRMLS_CC);
	if(host == NULL){
		RETVAL_NULL();
		return;
	}

	requsetUri = zend_read_static_property(CRouteCe,ZEND_STRL("requsetUri"),0 TSRMLS_CC);
	convert_to_string(requsetUri);
	htmlspecialchars(Z_STRVAL_P(requsetUri),&filter);


	strcat2(&url,"http://",host,filter,NULL);
	RETVAL_STRING(url,1);
	efree(url);
	efree(filter);
}

PHP_METHOD(CRequest,getUri)
{
	zval *requsetUri;
	char	*filter;

	requsetUri = zend_read_static_property(CRouteCe,ZEND_STRL("requsetUri"),0 TSRMLS_CC);
	htmlspecialchars(Z_STRVAL_P(requsetUri),&filter);
	RETVAL_STRING(filter,1);
	efree(filter);
}

PHP_METHOD(CRequest,getIp)
{
	char *clientIp,
		 *remoteAdder;

	getServerParam("HTTP_CLIENT_IP",&clientIp TSRMLS_CC);

	if(clientIp != NULL){
		ZVAL_STRING(return_value,clientIp,1);
		efree(clientIp);
		return; 
	}

	getServerParam("REMOTE_ADDR",&remoteAdder TSRMLS_CC);
	if(remoteAdder != NULL){
		ZVAL_STRING(return_value,remoteAdder,1);
		efree(remoteAdder);
	}else{
		ZVAL_STRING(return_value,"0.0.0.0",1);
	}
}

PHP_METHOD(CRequest,getPreUrl)
{
	char *perUrl;
	getServerParam("HTTP_REFERER",&perUrl TSRMLS_CC);
	if(perUrl == NULL){
		RETVAL_NULL();
		return;
	}
	ZVAL_STRING(return_value,perUrl,1);
	efree(perUrl);
}

PHP_METHOD(CRequest,getAgent)
{
	char *perUrl;
	getServerParam("HTTP_USER_AGENT",&perUrl TSRMLS_CC);
	if(perUrl == NULL){
		RETVAL_NULL();
		return;
	}
	ZVAL_STRING(return_value,perUrl,1);
	efree(perUrl);
}

PHP_METHOD(CRequest,getHost)
{
	char	*key,
			*host,
			*port = "",
			hostRetrun[10240];
	int		keyLen = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|s",&key,&keyLen) == FAILURE){
		RETURN_NULL();
	}

	if(keyLen == 0){
		key = "http";
	}


	getServerParam("HTTP_X_FORWARDED_HOST",&host TSRMLS_CC);
	if(host == NULL){
		getServerParam("HTTP_HOST",&host TSRMLS_CC);
	}

	if(host == NULL){
		RETVAL_NULL();
		return;
	}

	sprintf(hostRetrun,"%s%s%s",key,"://",host);
	RETVAL_STRING(hostRetrun,1);
	efree(host);
}

PHP_METHOD(CRequest,getStartTime)
{
	zval	**startTimeZval,
			**startTime;

	if(zend_hash_find(&EG(symbol_table),"SYSTEM_INIT",strlen("SYSTEM_INIT")+1,(void**)&startTimeZval) == SUCCESS && IS_ARRAY == Z_TYPE_PP(startTimeZval)){

		if(zend_hash_find(Z_ARRVAL_PP(startTimeZval),"frameBegin",strlen("frameBegin")+1,(void**)&startTime) == SUCCESS && IS_DOUBLE == Z_TYPE_PP(startTime)){
			RETVAL_DOUBLE(Z_DVAL_PP(startTime));
			return;
		}
	}
	
	RETVAL_NULL();
}

PHP_METHOD(CRequest,getRegisterEventTime)
{
	zval	**startTimeZval,
			**startTime;

	if(zend_hash_find(&EG(symbol_table),"SYSTEM_INIT",strlen("SYSTEM_INIT")+1,(void**)&startTimeZval) == SUCCESS && IS_ARRAY == Z_TYPE_PP(startTimeZval)){

		if(zend_hash_find(Z_ARRVAL_PP(startTimeZval),"registerEvent",strlen("registerEvent")+1,(void**)&startTime) == SUCCESS && IS_DOUBLE == Z_TYPE_PP(startTime)){
			RETVAL_DOUBLE(Z_DVAL_PP(startTime));
			return;
		}
	}
	
	RETVAL_NULL();
}

PHP_METHOD(CRequest,getErrorMessage)
{

}

PHP_METHOD(CRequest,getPath)
{
	RETVAL_STRING("",1);
}

PHP_METHOD(CRequest,createUrl)
{
	zval *paramsList,
		 *otherParams,
		 *otherParamsCopy;

	char *url = "";

	//��ȡ����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"a|z",&paramsList,&otherParams) == FAILURE){
		RETVAL_FALSE;
		return;
	}

	CRouteParse_url(paramsList,&url TSRMLS_CC);
	RETVAL_STRING(url,1);
	efree(url);
}

PHP_METHOD(CRequest,disablePOST)
{
}

PHP_METHOD(CRequest,disableGET)
{
}

PHP_METHOD(CRequest,isCli)
{
	zval	*sapiZval;

	if(zend_hash_find(EG(zend_constants),"PHP_SAPI",strlen("PHP_SAPI")+1,(void**)&sapiZval) == SUCCESS && strcmp(Z_STRVAL_P(sapiZval),"cli") == 0){
		RETVAL_TRUE;
	}else{
		RETVAL_FALSE;
	}
}

//�ж��Ƿ�Ϊwap��ʽ������
PHP_METHOD(CRequest,isWap)
{

	//�����HTTP_X_WAP_PROFILE��һ�����ƶ��豸 
	MODULE_BEGIN 
		char *httpWapProfile = NULL;
		getServerParam("HTTP_X_WAP_PROFILE",&httpWapProfile TSRMLS_CC);
		if(httpWapProfile != NULL){
			if(strlen(httpWapProfile) > 0){
				RETVAL_TRUE;
				efree(httpWapProfile);
				return;
			}
			efree(httpWapProfile);
		}

	MODULE_END


	// ���via��Ϣ����wap��һ�����ƶ��豸,���ַ����̻����θ���Ϣ  
	MODULE_BEGIN
		char *httpVia = NULL;
		getServerParam("HTTP_VIA",&httpVia TSRMLS_CC);
		if(httpVia != NULL){

			//����wap
			php_strtolower(httpVia,strlen(httpVia)+1);
			if(strstr(httpVia,"wap") != NULL){
				RETVAL_TRUE;
				efree(httpVia);
				return;
			}
			efree(httpVia);
		}
	MODULE_END


	//�ж�UserAgent�а����ֻ�Ʒ����Ϣ
	MODULE_BEGIN
		char *userAgent = NULL;
		getServerParam("HTTP_USER_AGENT",&userAgent TSRMLS_CC);
		if(userAgent != NULL){
			char *clientKey = "/(nokia|sony|ericsson|mot|samsung|htc|sgh|lg|sharp|sie-|philips|panasonic|alcatel|lenovo|iphone|ipod|blackberry|meizu|android|netfront|symbian|ucweb|windowsce|palm|operamini|operamobi|openwave|nexusone|cldc|midp|wap|mobile)/i";
			zval *matchArr;

			php_strtolower(userAgent,strlen(userAgent)+1);
			if(1 == preg_match(clientKey,userAgent,&matchArr)){
				zval_ptr_dtor(&matchArr);
				efree(userAgent);
				RETVAL_TRUE;
				return;
			}
			zval_ptr_dtor(&matchArr);
			efree(userAgent);
		}
	MODULE_END


	//�������� ����FLASE
	RETVAL_FALSE;
}

//����web������URL��дʱ���ӵ�GET����
void CRquest_fixWebServerRewriteParams(TSRMLS_D){

	zval **getDataZval;
	if(zend_hash_find(&EG(symbol_table), ZEND_STRS("_GET"), (void **)&getDataZval) == SUCCESS){
		if(IS_ARRAY == Z_TYPE_PP(getDataZval)){
			int		i = 0,
					paramsNum = 0;
			char	*key;
			ulong	ikey;
			zend_hash_internal_pointer_reset(Z_ARRVAL_PP(getDataZval));
			paramsNum = zend_hash_num_elements(Z_ARRVAL_PP(getDataZval));
			for(i = 0 ; i < paramsNum ; i++){

				if(HASH_KEY_IS_STRING == zend_hash_get_current_key_type(Z_ARRVAL_PP(getDataZval))){
					zend_hash_get_current_key(Z_ARRVAL_PP(getDataZval),&key,&ikey,0);
					zend_hash_del(Z_ARRVAL_PP(getDataZval),key,strlen(key)+1);
				}else if(HASH_KEY_IS_LONG == zend_hash_get_current_key_type(Z_ARRVAL_PP(getDataZval))){
					zend_hash_get_current_key(Z_ARRVAL_PP(getDataZval),&key,&ikey,0);
					zend_hash_index_del(Z_ARRVAL_PP(getDataZval),ikey);
				}
			}
		}
	}
}

//����ִ������
void CRequest_run(zval *object,zval **getRouteZval TSRMLS_DC)
{
	zval	*routeZval,
			*giveRouteZval,
			*routeInstance,
			*reReadRouteZval,
			*controllerObject,
			*getRouteReturn;

	//·�ɶ���
	CRoute_getInstance(&routeInstance TSRMLS_CC);

	//����ӦURL��д�������get����
	CRquest_fixWebServerRewriteParams(TSRMLS_C);

	//����·��֮ǰ��HOOKS_ROUTE_START����
	MODULE_BEGIN
		zval	*paramsList[1],
				param1;
		paramsList[0] = &param1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_ZVAL(paramsList[0],routeInstance,1,0);
		CHooks_callHooks("HOOKS_ROUTE_START",paramsList,1 TSRMLS_CC);
		zval_ptr_dtor(&routeInstance);
		zval_ptr_dtor(&paramsList[0]);
	MODULE_END

	//����·��
	CRouteParse_getRoute(&getRouteReturn TSRMLS_CC);
	MAKE_STD_ZVAL(*getRouteZval);
	ZVAL_ZVAL(*getRouteZval,getRouteReturn,1,0);

	//��ȡ·�ɶ���
	CRequest_checkActionPreFix(getRouteReturn,&routeZval TSRMLS_CC);

	//����·�ɺ��HOOKS_ROUTE_END����
	MODULE_BEGIN
		zval	*paramsList[1],
				param1;
		paramsList[0] = &param1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_ZVAL(paramsList[0],routeZval,1,0);
		CHooks_callHooks("HOOKS_ROUTE_END",paramsList,1 TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
	MODULE_END

	//��������·�� ���¶�ȡrouteZval��ֵ���õ��ں�CMyRoute�ṹ��
	reReadRouteZval = zend_read_property(CRouteCe,routeZval,ZEND_STRL("controller"),1 TSRMLS_CC);
	if(IS_STRING == Z_TYPE_P(reReadRouteZval) && strlen(Z_STRVAL_P(reReadRouteZval)) > 0 ){
		zend_update_static_property_string(CRouteCe,ZEND_STRL("thisController"),Z_STRVAL_P(reReadRouteZval) TSRMLS_CC);
	}else{
		//�����쳣
		zend_throw_exception(CRouteExceptionCe, "[PluginException] Registered to[HOOKS_ROUTE_END] function in the call CRoute::getInstance()->setController(string controllerName) method when the parameter type errors", 12001 TSRMLS_CC);
	}

	reReadRouteZval = zend_read_property(CRouteCe,routeZval,ZEND_STRL("action"),1 TSRMLS_CC);
	if(IS_STRING == Z_TYPE_P(reReadRouteZval) && strlen(Z_STRVAL_P(reReadRouteZval)) > 0 ){
		zend_update_static_property_string(CRouteCe,ZEND_STRL("thisAction"),Z_STRVAL_P(reReadRouteZval) TSRMLS_CC);
	}else{
		//�����쳣
		zend_throw_exception(CRouteExceptionCe, "[PluginException] Registered to[HOOKS_ROUTE_END] function in the call CRoute::getInstance()->setAction(string actionName) method when the parameter type errors", 12001 TSRMLS_CC);
	}

	reReadRouteZval = zend_read_property(CRouteCe,routeZval,ZEND_STRL("module"),1 TSRMLS_CC);
	if(IS_STRING == Z_TYPE_P(reReadRouteZval) && strlen(Z_STRVAL_P(reReadRouteZval)) > 0 ){
		zend_update_static_property_string(CRouteCe,ZEND_STRL("thisModule"),Z_STRVAL_P(reReadRouteZval) TSRMLS_CC);
	}else{
		//�����쳣
		zend_throw_exception(CRouteExceptionCe, "[PluginException] Registered to[HOOKS_ROUTE_END] function in the call CRoute::getInstance()->setModule(string moduleName) method when the parameter type errors", 12001 TSRMLS_CC);
	}

	//ִ��·��
	MAKE_STD_ZVAL(giveRouteZval);
	ZVAL_ZVAL(giveRouteZval,routeZval,1,0);

	CRequest_routeDoing(giveRouteZval TSRMLS_CC);

	controllerObject = zend_read_static_property(CRouteCe,ZEND_STRL("controllerObject"),1 TSRMLS_CC);

	//������������ʼ��HOOKS_CONTROLLER_INIT����
	MODULE_BEGIN
		zval	*paramsList[1],
				param1;
		paramsList[0] = &param1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_ZVAL(paramsList[0],controllerObject,1,0);
		CHooks_callHooks("HOOKS_CONTROLLER_INIT",paramsList,1 TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
	MODULE_END

	//ִ�з���
	CRequest_execAction(giveRouteZval,object TSRMLS_CC);

	//����ִ�з�����HOOKS_ACTION_INIT����
	MODULE_BEGIN
		zval	*paramsList[1],
				param1;
		paramsList[0] = &param1;
		MAKE_STD_ZVAL(paramsList[0]);
		ZVAL_ZVAL(paramsList[0],controllerObject,1,0);
		CHooks_callHooks("HOOKS_ACTION_INIT",paramsList,1 TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
	MODULE_END

	zval_ptr_dtor(&routeZval);
	zval_ptr_dtor(&getRouteReturn);
	zval_ptr_dtor(&giveRouteZval);
}

//����ִ������
PHP_METHOD(CRequest,run)
{
	zval	*routeDataZval;
	
	//����·��
	CRequest_run(getThis(),&routeDataZval TSRMLS_CC);
	zval_ptr_dtor(&routeDataZval);	
}


//��������
PHP_METHOD(CRequest,end)
{
	
}


PHP_METHOD(CRequest,getRequestMethod)
{
	char	*requestMethod;



	getServerParam("REQUEST_METHOD",&requestMethod TSRMLS_CC);
	if(requestMethod == NULL){
		zval	*sapiZval;
		if(zend_hash_find(EG(zend_constants),"PHP_SAPI",strlen("PHP_SAPI")+1,(void**)&sapiZval) == SUCCESS && strcmp(Z_STRVAL_P(sapiZval),"cli") == 0){
			RETURN_STRING("CLI",1);
		}

		RETURN_FALSE;
	}

	RETVAL_STRING(requestMethod,0);
}