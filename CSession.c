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
#include "php_CSession.h"


//zend�෽��
zend_function_entry CSession_functions[] = {
	PHP_ME(CSession,set,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CSession,get,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CSession,del,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};

//ģ�鱻����ʱ
CMYFRAME_REGISTER_CLASS_RUN(CSession)
{
	//ע��CWebApp��
	zend_class_entry funCe;

	INIT_CLASS_ENTRY(funCe,"CSession",CSession_functions);
	CSessionCe = zend_register_internal_class(&funCe TSRMLS_CC);

	return SUCCESS;
}

//�෽��:����Ӧ�ö���
PHP_METHOD(CSession,set)
{
	char	*key;
	int		keyLen;
	zval	*val,
			**array,
			*saveVal;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sz",&key,&keyLen,&val) == FAILURE){
		RETVAL_FALSE;
		return;
	}

	MAKE_STD_ZVAL(saveVal);
	ZVAL_ZVAL(saveVal,val,1,0);

	if( zend_hash_find(&EG(symbol_table),"_SESSION",sizeof("_SESSION"),(void**)&array) != SUCCESS ){
		RETVAL_FALSE;
		return;
	}

	if(IS_ARRAY != Z_TYPE_PP(array)){
		array_init(*array);
	}

	zend_hash_update(Z_ARRVAL_PP(array),key,strlen(key)+1,&saveVal,sizeof(zval*),NULL);
	RETVAL_TRUE;
}

PHP_METHOD(CSession,get)
{
	char	*key;
	int		keyLen;
	zval	**array,
			*saveVal;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&key,&keyLen) == FAILURE){
		RETVAL_FALSE;
		return;
	}

	if( zend_hash_find(&EG(symbol_table),"_SESSION",strlen("_SESSION")+1,(void**)&array) != SUCCESS){
		RETVAL_FALSE;
		return;
	}

	if(IS_ARRAY != Z_TYPE_PP(array) ){
		RETVAL_FALSE;
		return;
	}


	//����������ݷ���null
	if(IS_ARRAY != Z_TYPE_PP(array)){
		RETVAL_NULL();
		return;
	}

	//��key��ֱ�ӷ���
	if(zend_hash_exists(Z_ARRVAL_PP(array),key,strlen(key)+1)){
		zval **keyVal;
		if(SUCCESS == zend_hash_find(Z_ARRVAL_PP(array),key,strlen(key)+1,(void**)&keyVal)){
			RETVAL_ZVAL(*keyVal,1,0);
			return;
		}
	}

	//�ж��Ƿ���ڷָ��.
	if(strstr((key),".") != NULL){

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
			ZVAL_ZVAL(currentData,*array,1,0);

			configTableNum = zend_hash_num_elements(Z_ARRVAL_P(cutArr));
			zend_hash_internal_pointer_reset(Z_ARRVAL_P(cutArr));

			for( n = 0 ; n < configTableNum ; n++){

				zend_hash_get_current_data(Z_ARRVAL_P(cutArr),(void**)&nData);
				
				if(IS_ARRAY != Z_TYPE_P(currentData)){
					zval_ptr_dtor(&cutArr);
					zval_ptr_dtor(&currentData);
					RETURN_NULL();
				}

				//�����ڸò���򷵻�null
				if(!zend_hash_exists(Z_ARRVAL_P(currentData),Z_STRVAL_PP(nData),strlen(Z_STRVAL_PP(nData)) + 1)){
					zval_ptr_dtor(&cutArr);
					zval_ptr_dtor(&currentData);
					RETURN_NULL();
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

			RETVAL_ZVAL(currentData,1,0);
			zval_ptr_dtor(&currentData);
			zval_ptr_dtor(&cutArr);
			return;
		}
		zval_ptr_dtor(&cutArr);
	}

	RETVAL_NULL();

}

PHP_METHOD(CSession,del)
{
	char	*key;
	int		keyLen;
	zval	**array;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&key,&keyLen) == FAILURE){
		RETVAL_FALSE;
		return;
	}

	if( zend_hash_find(&EG(symbol_table),"_SESSION",sizeof("_SESSION"),(void**)&array) != SUCCESS ){
		RETVAL_FALSE;
		return;
	}

	if(zend_hash_exists(Z_ARRVAL_PP(array),key,strlen(key)+1)){	
		zend_hash_del(Z_ARRVAL_PP(array),key,strlen(key)+1);
		RETVAL_TRUE;
	}else{
		RETVAL_FALSE;
	}
}
