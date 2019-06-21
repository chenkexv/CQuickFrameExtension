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
#include "php_CConfig.h"
#include "php_CWebApp.h"
#include "php_CException.h"

//zend�෽��
zend_function_entry CConfig_functions[] = {
	PHP_ME(CConfig,getInstance,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CConfig,__construct,NULL,ZEND_ACC_CTOR | ZEND_ACC_PUBLIC)
	PHP_ME(CConfig,load,NULL, ZEND_ACC_PUBLIC)
	PHP_ME(CConfig,set,NULL, ZEND_ACC_PUBLIC)
	PHP_ME(CConfig,get,NULL, ZEND_ACC_PUBLIC)
	PHP_ME(CConfig,setConfigs,NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

//ģ�鱻����ʱ
CMYFRAME_REGISTER_CLASS_RUN(CConfig)
{
	//ע����
	zend_class_entry funCe;
	INIT_CLASS_ENTRY(funCe,"CConfig",CConfig_functions);
	CConfigCe = zend_register_internal_class(&funCe TSRMLS_CC);

	//��������
	zend_declare_property_null(CConfigCe, ZEND_STRL("instance"),ZEND_ACC_PRIVATE|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(CConfigCe, ZEND_STRL("thisConfigData"),ZEND_ACC_PRIVATE TSRMLS_CC);
	
	return SUCCESS;
}

int CConfig_setConfigs(zval *params,zval *object TSRMLS_DC){

	if(IS_ARRAY != Z_TYPE_P(params)){
		return FAILURE;
	}

	zend_update_property(CConfigCe,object,ZEND_STRL("thisConfigData"),params TSRMLS_CC);
	return SUCCESS;
}

//���ò���
PHP_METHOD(CConfig,setConfigs){

	zval *bootParams;

	//��ȡ����
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&bootParams) == FAILURE){
		return;
	}

	if(SUCCESS == CConfig_setConfigs(bootParams,getThis() TSRMLS_CC)){
		RETVAL_TRUE;
	}else{
		RETVAL_FALSE;
	}

}

//������������
int CConfig_getInstance(char *configFileName,zval **returnZval TSRMLS_DC)
{
	zval	*selfInstace = NULL,
			*funResultZval,
			**instaceSaveZval;

	int		configFileNameLen,
			hasExistConfig = 0;


	//��ȡ�൥�ж���
	selfInstace = zend_read_static_property(CConfigCe,ZEND_STRL("instance"),1 TSRMLS_CC);

	//���ΪNULL�����ΪZvalHashtable
	if(IS_ARRAY != Z_TYPE_P(selfInstace)){
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		zend_update_static_property(CConfigCe,ZEND_STRL("instance"),saveArray TSRMLS_CC);
		zval_ptr_dtor(&saveArray);
		selfInstace = zend_read_static_property(CConfigCe,ZEND_STRL("instance"),1 TSRMLS_CC);
	}

	//�жϵ��ж����д���config��key
	hasExistConfig = zend_hash_exists(Z_ARRVAL_P(selfInstace), configFileName, strlen(configFileName)+1);

	//�����ڳ���ʵ����
	if(0 == hasExistConfig){
		
		zval	*object = NULL,
				constructReturn,
				constructVal,
				*instaceZval = NULL,
				*toReturnObject = NULL;

		zend_class_entry **configClassCepp = NULL;
		zend_hash_find(EG(class_table),"cconfig",strlen("cconfig")+1,(void**)&configClassCepp);
		if(configClassCepp == NULL){
			php_error_docref(NULL TSRMLS_CC,E_ERROR,"[CQuickFrameworkFatalError] Framework can not get a pointer from zend_hash_find");
			return;
		}
		MAKE_STD_ZVAL(object);
		object_init_ex(object,*configClassCepp);

		//ִ���乹���� ���������
		if ((*configClassCepp)->constructor) {
			zval *params[1],
				 param1;

			params[0] = &param1;
			INIT_ZVAL(constructVal);
			MAKE_STD_ZVAL(params[0]);
			ZVAL_STRING(params[0],configFileName,1);
			ZVAL_STRING(&constructVal, (*configClassCepp)->constructor->common.function_name, 0);
			call_user_function(NULL, &object, &constructVal, &constructReturn, 1, params TSRMLS_CC);
			zval_ptr_dtor(&params[0]);
			zval_dtor(&constructReturn);
		}

		//������������ֵ����instance��̬����
		add_assoc_zval(selfInstace,configFileName,object);
		zend_update_static_property(CConfigCe,ZEND_STRL("instance"),selfInstace TSRMLS_CC);

		MAKE_STD_ZVAL(*returnZval);
		ZVAL_ZVAL(*returnZval,object,1,0);
		return;
	}

	//ֱ��ȡinstace��̬�����еķ���ֵ
	if(SUCCESS == zend_hash_find(Z_ARRVAL_P(selfInstace),configFileName,strlen(configFileName)+1,(void**)&instaceSaveZval) ){
		MAKE_STD_ZVAL(*returnZval);
		ZVAL_ZVAL(*returnZval,*instaceSaveZval,1,0);
	}
}

//�෽��:����Ӧ�ö���
PHP_METHOD(CConfig,getInstance)
{

	char	*configFileName = NULL;

	int		configFileNameLen = 0;

	zval	*resultZval;

	//��ȡ���ò���
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|s",&configFileName,&configFileNameLen) == FAILURE){
		zend_throw_exception(CClassNotFoundExceptionCe, "[CClassNotFoundException] Class [CConfig] Not Found", 10001 TSRMLS_CC);
		return;
	}

	if(configFileName == NULL){
		configFileName = "main";
	}

	//����C API
	CConfig_getInstance(configFileName,&resultZval TSRMLS_CC);
	ZVAL_ZVAL(return_value,resultZval,1,1);
}

//���캯��
PHP_METHOD(CConfig,__construct)
{
	char	*configFileName;
	char	*configFilePath,
			*codePath = NULL,
			*saveConfigName = NULL;

	int		configFileNameLen;

	zval	*loadFileZval,
			*codePathConstants;


	//��ȡ���ò���
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&configFileName,&configFileNameLen) == FAILURE){
		zend_throw_exception(CClassNotFoundExceptionCe, "[CClassNotFoundException] Class [CConfig] Not Found", 10001 TSRMLS_CC);
		return;
	}


	codePathConstants = zend_read_static_property(CWebAppCe,ZEND_STRL("app_path"),1 TSRMLS_CC);

	if(IS_STRING == Z_TYPE_P(codePathConstants) && Z_STRLEN_P(codePathConstants) > 0){
		spprintf(&codePath,0,"%s%s",Z_STRVAL_P(codePathConstants),"/application");
	}else{
		codePath = estrdup("/application");
	}


	//�����ļ�·��
	strcat2(&configFilePath,codePath,"/configs/",configFileName,".php",NULL);
	efree(codePath);

	//���øö�����configName��ֵ
	saveConfigName = estrdup(configFileName);
	zend_update_property_string(CConfigCe,getThis(),ZEND_STRL("configName"),saveConfigName TSRMLS_CC);
	efree(saveConfigName);

	//���Ե���CLoad�������ļ�����
	if(SUCCESS != fileExist(configFilePath)){
		efree(configFilePath);
		return;
	}

	CLoader_import(configFileName,configFilePath,&loadFileZval TSRMLS_CC);

	//��thisConfigData��ֵ���ó������ļ�
	zend_update_property(CConfigCe,getThis(),ZEND_STRL("thisConfigData"),loadFileZval TSRMLS_CC);
	zval_ptr_dtor(&loadFileZval);
	efree(configFilePath);
}

//��ϵͳini�л�ȡ�����ļ�
int CConfig_loadfromIni(char *key,zval **returnZval TSRMLS_DC){

	int		i,j;
	zend_ini_entry *ini_entry;;
	j = zend_hash_num_elements(EG(ini_directives));
	zend_hash_internal_pointer_reset(EG(ini_directives));
	for(i = 0 ; i < j ; i++){

		zend_hash_get_current_data(EG(ini_directives),(void**)&ini_entry);
		php_printf("%s<br>",ini_entry->value);
		zend_hash_move_forward(EG(ini_directives));
	}


}

int CConfig_loadIntKey(ulong key,zval *object,zval **returnZval TSRMLS_DC)
{
	zval	*configsData;

	MAKE_STD_ZVAL(*returnZval);

	//��ȡ�ö����е�thisConfigData����
	configsData = zend_read_property(CConfigCe,object,ZEND_STRL("thisConfigData"),0 TSRMLS_CC);

	//����������ݷ���null
	if(IS_ARRAY != Z_TYPE_P(configsData)){
		ZVAL_NULL(*returnZval);
		return FAILURE;
	}

	//��key��ֱ�ӷ���
	if(zend_hash_index_exists(Z_ARRVAL_P(configsData),key)){
		zval **keyVal;
		if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(configsData),key,(void**)&keyVal)){
			ZVAL_ZVAL(*returnZval,*keyVal,1,0);
			return SUCCESS;
		}
	}else{
		ZVAL_NULL(*returnZval);
		return FAILURE;
	}
}

//Load����
int CConfig_load(char *key,zval *object,zval **returnZval TSRMLS_DC)
{
	zval	*configsData;

	MAKE_STD_ZVAL(*returnZval);

	//��ȡ�ö����е�thisConfigData����
	configsData = zend_read_property(CConfigCe,object,ZEND_STRL("thisConfigData"),0 TSRMLS_CC);

	//����������ݷ���null
	if(IS_ARRAY != Z_TYPE_P(configsData)){

		ZVAL_NULL(*returnZval);
		return FAILURE;
	}

	//��key��ֱ�ӷ���
	if(zend_hash_exists(Z_ARRVAL_P(configsData),key,strlen(key)+1)){
		zval **keyVal;
		if(SUCCESS == zend_hash_find(Z_ARRVAL_P(configsData),key,strlen(key)+1,(void**)&keyVal)){
			ZVAL_ZVAL(*returnZval,*keyVal,1,0);
			return SUCCESS;
		}
	}

	//�ж��Ƿ���ڷָ��.
	if(strstr(key,".") != NULL){

		//��key����.�ָ�
		zval *cutArr;
		php_explode(".",key,&cutArr);

		if(IS_ARRAY == Z_TYPE_P(cutArr) && zend_hash_num_elements(Z_ARRVAL_P(cutArr)) > 1){
			
			//��configsData�����Hashtable���б���
			int configTableNum,
				n;

			zval *currentData,
				 **nData,
				 **resetData;

			//��ǰֵ
			MAKE_STD_ZVAL(currentData);
			ZVAL_ZVAL(currentData,configsData,1,0);

			configTableNum = zend_hash_num_elements(Z_ARRVAL_P(cutArr));
			zend_hash_internal_pointer_reset(Z_ARRVAL_P(cutArr));

			for( n = 0 ; n < configTableNum ; n++){

				zend_hash_get_current_data(Z_ARRVAL_P(cutArr),(void**)&nData);
				
				if(IS_ARRAY != Z_TYPE_P(currentData)){
					ZVAL_NULL(*returnZval);
					zval_ptr_dtor(&cutArr);
					zval_ptr_dtor(&currentData);
					return FAILURE;
				}

				if(isdigitstr(Z_STRVAL_PP(nData))){
				
					int keyInt = toInt(Z_STRVAL_PP(nData));

					//�����ڸò���򷵻�null
					if(!zend_hash_index_exists(Z_ARRVAL_P(currentData),keyInt)){
						ZVAL_NULL(*returnZval);
						zval_ptr_dtor(&cutArr);
						zval_ptr_dtor(&currentData);
						return FAILURE;
					}

					//����cutArr��ֵ
					if(SUCCESS == zend_hash_index_find(Z_ARRVAL_P(currentData),keyInt,(void**)&resetData)){
						zval *tempZval;

						MAKE_STD_ZVAL(tempZval);
						ZVAL_ZVAL(tempZval,*resetData,1,0);
						zval_ptr_dtor(&currentData);

						MAKE_STD_ZVAL(currentData);
						ZVAL_ZVAL(currentData,tempZval,1,0);
						zval_ptr_dtor(&tempZval);
					}
					zend_hash_move_forward(Z_ARRVAL_P(cutArr));

				}else{

					//�����ڸò���򷵻�null
					if(!zend_hash_exists(Z_ARRVAL_P(currentData),Z_STRVAL_PP(nData),strlen(Z_STRVAL_PP(nData)) + 1)){
						ZVAL_NULL(*returnZval);
						zval_ptr_dtor(&cutArr);
						zval_ptr_dtor(&currentData);
						return FAILURE;
					}

					//����cutArr��ֵ
					if(SUCCESS == zend_hash_find(Z_ARRVAL_P(currentData),Z_STRVAL_PP(nData),Z_STRLEN_PP(nData) + 1,(void**)&resetData)){
						zval *tempZval;

						MAKE_STD_ZVAL(tempZval);
						ZVAL_ZVAL(tempZval,*resetData,1,0);
						zval_ptr_dtor(&currentData);

						MAKE_STD_ZVAL(currentData);
						ZVAL_ZVAL(currentData,tempZval,1,0);
						zval_ptr_dtor(&tempZval);
					}
					zend_hash_move_forward(Z_ARRVAL_P(cutArr));
				}
			}

			ZVAL_ZVAL(*returnZval,currentData,1,0);
			zval_ptr_dtor(&currentData);
			zval_ptr_dtor(&cutArr);
			return SUCCESS;
		}else{
			char *errMessage;
			strcat2(&errMessage,"[CQuickFrameworkException] Unable to configure key CMyFrame decomposition of configuration items when loading:",key,NULL);
			zend_throw_exception(CExceptionCe,errMessage, 10000 TSRMLS_CC);
			efree(errMessage);
			zval_ptr_dtor(&cutArr);
			return FAILURE;
		}
	}

	ZVAL_NULL(*returnZval);
	return FAILURE;

}

//Load����
PHP_METHOD(CConfig,load)
{
	char	*key;
	int		keyLen;

	zval	*returnZval,
			*defaultZval = NULL;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|z",&key,&keyLen,&defaultZval) == FAILURE){
		zend_throw_exception(CExceptionCe,"Call CConfig->load (), key must be a string", 10000 TSRMLS_CC);
		return;
	}

	//��ȡ������
	CConfig_load(key,getThis(),&returnZval TSRMLS_CC);
	if(IS_NULL == Z_TYPE_P(returnZval) && defaultZval != NULL){
		ZVAL_ZVAL(return_value,defaultZval,1,0);
	}else{
		ZVAL_ZVAL(return_value,returnZval,1,0);
	}
	zval_ptr_dtor(&returnZval);
}

PHP_METHOD(CConfig,get)
{
	char	*key;
	int		keyLen;

	zval	*returnZval;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&key,&keyLen) == FAILURE){
		zend_throw_exception(CExceptionCe,"Call CConfig->get (), key must be a string", 10000 TSRMLS_CC);
		return;
	}

	//��ȡ������
	CConfig_load(key,getThis(),&returnZval TSRMLS_CC);
	ZVAL_ZVAL(return_value,returnZval,1,1);
}

PHP_METHOD(CConfig,set)
{
	char	*key;
	int		keyLen;

	zval	*setVal,
			*configsData;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sz",&key,&keyLen,&setVal) == FAILURE){
		zend_throw_exception(CExceptionCe,"Call CConfig->set (), key must be a string,an object", 10000 TSRMLS_CC);
		return;
	}

	//��ȡ�ö����е�thisConfigData����
	configsData = zend_read_property(CConfigCe,getThis(),ZEND_STRL("thisConfigData"),0 TSRMLS_CC);

	if(IS_ARRAY == Z_TYPE_P(configsData)){
		zval *saveData;
		MAKE_STD_ZVAL(saveData);
		ZVAL_ZVAL(saveData,setVal,1,0);
		zend_hash_update(Z_ARRVAL_P(configsData),key,strlen(key)+1,&saveData,sizeof(zval*),NULL);
	}
}