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
#include "php_CModel.h"
#include "php_CException.h"
#include "php_CActiveRecord.h"


//zend�෽��
zend_function_entry CModel_functions[] = {
	PHP_ME(CModel,factory,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CModel,getInstance,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CModel,getPageLimit,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CModel,getPageRows,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CModel,setPageRows,NULL,ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

zend_function_entry CEmptyModel_functions[] = {
	PHP_ME(CEmptyModel,getTableName,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CEmptyModel,from,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CEmptyModel,setTableName,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CEmptyModel,tableName,NULL,ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

//ģ�鱻����ʱ
CMYFRAME_REGISTER_CLASS_RUN(CModel)
{
	zend_class_entry funCe;

	INIT_CLASS_ENTRY(funCe,"CModel",CModel_functions);
	CModelCe = zend_register_internal_class(&funCe TSRMLS_CC);

	//�����
	zend_declare_property_null(CModelCe, ZEND_STRL("modelList"),ZEND_ACC_PRIVATE|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_long(CModelCe, ZEND_STRL("pageRows"),20,ZEND_ACC_PUBLIC TSRMLS_CC);

	return SUCCESS;
}

CMYFRAME_REGISTER_CLASS_RUN(CEmptyModel)
{
	zend_class_entry funCe;
	INIT_CLASS_ENTRY(funCe,"CEmptyModel",CEmptyModel_functions);
	CEmptyModelCe = zend_register_internal_class_ex(&funCe,CActiveRecordCe,NULL TSRMLS_CC);

	//�������
	zend_declare_property_null(CEmptyModelCe, ZEND_STRL("_tableName"),ZEND_ACC_PRIVATE TSRMLS_CC);

	return SUCCESS;
}


//��������
void CModel_factory(char *modelName,zval **returnZval TSRMLS_DC)
{
	zval	*modelListZval,
			*modelObject;

	int		existModelObject;

	char	*trueModelName;

	//Сд��ģ������
	trueModelName = estrdup(modelName);
	php_strtolower(trueModelName,strlen(trueModelName)+1);

	MAKE_STD_ZVAL(*returnZval);

	//��ȡmodelList�����
	modelListZval = zend_read_static_property(CModelCe,ZEND_STRL("modelList"),0 TSRMLS_CC);
	if(IS_NULL == Z_TYPE_P(modelListZval)){
		zval *saveArray;
		MAKE_STD_ZVAL(saveArray);
		array_init(saveArray);
		zend_update_static_property(CModelCe,ZEND_STRL("modelList"),saveArray TSRMLS_CC);
		zval_ptr_dtor(&saveArray);
		modelListZval = zend_read_static_property(CModelCe,ZEND_STRL("modelList"),0 TSRMLS_CC);
	}

	//�жϱ������Ƿ���ڸ�ģ�͵Ķ���
	existModelObject = zend_hash_exists(Z_ARRVAL_P(modelListZval),modelName,strlen(modelName)+1 TSRMLS_CC);

	//�����ڳ��Ի�ȡ��ģ�Ͷ���
	if(0 == existModelObject){
		zend_class_entry	**modelCePP,
							*modelCe;
		//���Ҹ�ģ��
		if(zend_hash_find(EG(class_table),trueModelName,strlen(trueModelName)+1,(void**)&modelCePP) == FAILURE){
			//�Ҳ��������CLoader���������ļ�
			zval *canLoad;
			CLoader_load(modelName,&canLoad TSRMLS_CC);
			if(IS_BOOL == Z_TYPE_P(canLoad) && Z_LVAL_P(canLoad) == 1){
			}else{
				char errMessage[10240];
				sprintf(errMessage,"%s%s%s","[ModelException] Model [",modelName,"] does not exist or cannot be found");
				ZVAL_NULL(*returnZval);
				efree(trueModelName);
				zend_throw_exception(CModelExceptionCe, errMessage, "" TSRMLS_CC);
				return;
			}
			zval_ptr_dtor(&canLoad);
		}

		//��ȡģ����Ŀ
		if(zend_hash_find(EG(class_table),trueModelName,strlen(trueModelName)+1,(void**)&modelCePP) == FAILURE ){
			char errMessage[10240];
			sprintf(errMessage,"%s%s%s","[ModelException] Model [",modelName,"] does not exist or cannot be found");
			ZVAL_NULL(*returnZval);
			efree(trueModelName);
			zend_throw_exception(CModelExceptionCe, errMessage, "" TSRMLS_CC);
			return;
		}

		//ʵ������ģ��
		modelCe = *modelCePP;
		MAKE_STD_ZVAL(modelObject);
		object_init_ex(modelObject,modelCe);

		//ִ�й�����
		if (modelCe->constructor) {
			zval constructReturn;
			zval constructVal;
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal, modelCe->constructor->common.function_name, 0);
			call_user_function(NULL, &modelObject, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
			zval_dtor(&constructReturn);
		}

		//������modelList��
		ZVAL_ZVAL(*returnZval,modelObject,1,0);
		zend_hash_add(Z_ARRVAL_P(modelListZval),modelName,strlen(modelName)+1,&modelObject,sizeof(zval*),NULL);
		zend_update_static_property(CModelCe,ZEND_STRL("modelList"),modelListZval TSRMLS_CC);
		efree(trueModelName);
		return;
	}else{
		zval **saveModelZval;

		//ֱ�Ӵӱ�����ж�ȡ
		zend_hash_find(Z_ARRVAL_P(modelListZval),modelName,strlen(modelName)+1,(void**)&saveModelZval);
		ZVAL_ZVAL(*returnZval,*saveModelZval,1,0);
		efree(trueModelName);
		return;
	}
}

//��������
PHP_METHOD(CModel,factory)
{
	char	*modelName,
			*trueModelName;
	int		modelNameLen = 0;
	zval	*returnZval;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|s",&modelName,&modelNameLen) == FAILURE){
		zend_throw_exception(CModelExceptionCe, "[ModelException] Call the CModel factory methods fail", "" TSRMLS_CC);
		return;
	}

	//δָ��ģ�� ���ؿ�ģ��
	if(0 == modelNameLen){
		trueModelName = estrdup("CEmptyModel");
	}else{
		trueModelName = estrdup(modelName);
	}

	CModel_factory(trueModelName,&returnZval TSRMLS_CC);
	ZVAL_ZVAL(return_value,returnZval,1,0);
	zval_ptr_dtor(&returnZval);
	efree(trueModelName);
}

//����CEmptyModel
PHP_METHOD(CModel,getInstance)
{
	zval *returnZval;

	CModel_factory("CEmptyModel",&returnZval TSRMLS_CC);
	ZVAL_ZVAL(return_value,returnZval,1,1);
}

PHP_METHOD(CModel,getPageLimit)
{
	zval	*pageZval;
	int		pageRow,
			blimit;
	long	nowPage = 1;
	char	limit[10240];

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|l",&nowPage) == FAILURE){
		nowPage = 1;
	}


	pageZval = zend_read_property(CEmptyModelCe,getThis(),ZEND_STRL("pageRows"),0 TSRMLS_CC);
	if(IS_LONG == Z_TYPE_P(pageZval) && Z_LVAL_P(pageZval) > 0 ){
		pageRow = Z_LVAL_P(pageZval);
	}else{
		pageRow = 20;
	}

	//���������ҳ�뷶Χ
	if(nowPage <= 0){
		nowPage = 1;
	}

	blimit = (nowPage - 1) * pageRow;
	sprintf(limit,"%d%s%d",blimit,",",pageRow);
	RETVAL_STRING(limit,1);
}

//���ر���
PHP_METHOD(CEmptyModel,getTableName)
{
	zval *tableName;
	tableName = zend_read_property(CEmptyModelCe,getThis(),ZEND_STRL("_tableName"),0 TSRMLS_CC);
	if(IS_STRING == Z_TYPE_P(tableName)){
		RETVAL_STRING(Z_STRVAL_P(tableName),1);
		return;
	}
	RETVAL_NULL();
}

PHP_METHOD(CEmptyModel,tableName)
{
	zval *tableName;
	tableName = zend_read_property(CEmptyModelCe,getThis(),ZEND_STRL("_tableName"),0 TSRMLS_CC);
	if(IS_STRING == Z_TYPE_P(tableName)){
		RETVAL_STRING(Z_STRVAL_P(tableName),1);
		return;
	}
	RETVAL_NULL();
}

//�������
PHP_METHOD(CEmptyModel,from)
{
	char	*tableName;
	int		tableNameLen;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&tableName,&tableNameLen) == FAILURE){
		RETVAL_FALSE;
		return;
	}
	//���������
	zend_update_property_string(CEmptyModelCe,getThis(),ZEND_STRL("_tableName"),tableName TSRMLS_CC);
	ZVAL_ZVAL(return_value,getThis(),1,0);
}

//�������
PHP_METHOD(CEmptyModel,setTableName)
{
	char	*tableName;
	int		tableNameLen;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&tableName,&tableNameLen) == FAILURE){
		RETVAL_FALSE;
		return;
	}
	//���������
	zend_update_property_string(CEmptyModelCe,getThis(),ZEND_STRL("_tableName"),tableName TSRMLS_CC);
	ZVAL_ZVAL(return_value,getThis(),1,0);
}

PHP_METHOD(CModel,getPageRows)
{
	zval *pagerows;
	pagerows = zend_read_property(CEmptyModelCe,getThis(),ZEND_STRL("pageRows"),0 TSRMLS_CC);
	if(IS_LONG == Z_TYPE_P(pagerows)){
		RETVAL_LONG(Z_LVAL_P(pagerows));
		return;
	}
	RETVAL_LONG(0);
}

PHP_METHOD(CModel,setPageRows)
{

	long pageSet = 20;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"l",&pageSet) == FAILURE){
		RETVAL_FALSE;
		return;
	}
	//���������
	zend_update_property_long(CModelCe,getThis(),ZEND_STRL("pageRows"),pageSet TSRMLS_CC);
	ZVAL_ZVAL(return_value,getThis(),1,0);
}