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
#include "ext/standard/php_smart_str_public.h"
#include "ext/standard/php_smart_str.h"


#include "php_CQuickFramework.h"
#include "php_CMathModel.h"
#include "php_CException.h"
#include "php_CWebApp.h"



//zend�෽��
zend_function_entry CMathModel_functions[] = {
	PHP_ME(CMathModel,__construct,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(CMathModel,getInstance,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(CMathModel,setData,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CMathModel,getMathMode,NULL,ZEND_ACC_PRIVATE)
	PHP_ME(CMathModel,get,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CMathModel,getParams,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CMathModel,getModelType,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CMathModel,getFunction,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CMathModel,getR2,NULL,ZEND_ACC_PUBLIC)
	PHP_ME(CMathModel,getLineFunction,NULL,ZEND_ACC_PRIVATE)
	PHP_ME(CMathModel,getLnFunction,NULL,ZEND_ACC_PRIVATE)
	PHP_ME(CMathModel,getExpFunction,NULL,ZEND_ACC_PRIVATE)
	PHP_ME(CMathModel,getPowFunction,NULL,ZEND_ACC_PRIVATE)

	{NULL, NULL, NULL}
};

//ģ�鱻����ʱ
CMYFRAME_REGISTER_CLASS_RUN(CMathModel)
{
	zend_class_entry funCe;

	INIT_CLASS_ENTRY(funCe,"CMathModel",CMathModel_functions);
	CMathModelCe = zend_register_internal_class(&funCe TSRMLS_CC);

	//ȫ�ֱ���
	zend_declare_property_null(CMathModelCe, ZEND_STRL("instance"),ZEND_ACC_PRIVATE | ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(CMathModelCe, ZEND_STRL("modelType"),ZEND_ACC_PRIVATE TSRMLS_CC);

	//�ݺ���
	zend_declare_property_long(CMathModelCe, ZEND_STRL("powA"),1,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CMathModelCe, ZEND_STRL("powB"),1,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CMathModelCe, ZEND_STRL("powR"),1,ZEND_ACC_PRIVATE TSRMLS_CC);

	//ָ������
	zend_declare_property_long(CMathModelCe, ZEND_STRL("expA"),1,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CMathModelCe, ZEND_STRL("expB"),1,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CMathModelCe, ZEND_STRL("expR"),1,ZEND_ACC_PRIVATE TSRMLS_CC);

	//��������
	zend_declare_property_long(CMathModelCe, ZEND_STRL("lnA"),1,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CMathModelCe, ZEND_STRL("lnB"),1,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CMathModelCe, ZEND_STRL("lnR"),1,ZEND_ACC_PRIVATE TSRMLS_CC);

	//���Ժ���
	zend_declare_property_long(CMathModelCe, ZEND_STRL("lineA"),1,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CMathModelCe, ZEND_STRL("lineB"),1,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CMathModelCe, ZEND_STRL("lineR"),1,ZEND_ACC_PRIVATE TSRMLS_CC);

	return SUCCESS;
}

int CMathModel_getInstance(zval **returnZval TSRMLS_DC)
{
	zval	*instanceZval,
		    *backZval;

	//��ȡ��̬instace����
	instanceZval = zend_read_static_property(CMathModelCe,ZEND_STRL("instance"),0 TSRMLS_CC);

	//Ϊ��ʱ��ʵ��������
	if(IS_NULL == Z_TYPE_P(instanceZval) ){
		zval				*object,
							*saveObject;


		//ʵ�����ò��
		MAKE_STD_ZVAL(object);
		object_init_ex(object,CMathModelCe);

		//ִ�й�����
		if (CMathModelCe->constructor) {
			zval constructReturn;
			zval constructVal;
			INIT_ZVAL(constructVal);
			ZVAL_STRING(&constructVal, CMathModelCe->constructor->common.function_name, 0);
			call_user_function(NULL, &object, &constructVal, &constructReturn, 0, NULL TSRMLS_CC);
			zval_dtor(&constructReturn);
		}

		//������󱣴���instance��̬����
		zend_update_static_property(CMathModelCe,ZEND_STRL("instance"),object TSRMLS_CC);
		
		//����
		MAKE_STD_ZVAL(*returnZval);
		ZVAL_ZVAL(*returnZval,object,1,1);
		return SUCCESS;
	}

	MAKE_STD_ZVAL(*returnZval);
	ZVAL_ZVAL(*returnZval,instanceZval,1,0);
	return SUCCESS;
}

PHP_METHOD(CMathModel,getInstance)
{
	zval *instanceZval;

	CMathModel_getInstance(&instanceZval TSRMLS_CC);
	ZVAL_ZVAL(return_value,instanceZval,1,1);
}

PHP_METHOD(CMathModel,__construct){}

PHP_METHOD(CMathModel,setData){}
PHP_METHOD(CMathModel,getMathMode){}
PHP_METHOD(CMathModel,get){}
PHP_METHOD(CMathModel,getParams){}
PHP_METHOD(CMathModel,getModelType){}
PHP_METHOD(CMathModel,getFunction){}
PHP_METHOD(CMathModel,getR2){}
PHP_METHOD(CMathModel,getLineFunction){}
PHP_METHOD(CMathModel,getLnFunction){}
PHP_METHOD(CMathModel,getExpFunction){}
PHP_METHOD(CMathModel,getPowFunction){}