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

//zend类对象
zend_class_entry	*CValidateCe;


//类方法:创建应用
PHP_METHOD(CValidate,isPhone);
PHP_METHOD(CValidate,isEmail);
PHP_METHOD(CValidate,isBetween);
PHP_METHOD(CValidate,isIDCard);
PHP_METHOD(CValidate,isNumber);
PHP_METHOD(CValidate,isUrl);
PHP_METHOD(CValidate,isSimpleString);
PHP_METHOD(CValidate,isComplexString);
PHP_METHOD(CValidate,checkMustField);
PHP_METHOD(CValidate,checkMustKeys);
PHP_METHOD(CValidate,getInstance);
PHP_METHOD(CValidate,check);
PHP_METHOD(CValidate,set);
PHP_METHOD(CValidate,__construct);
PHP_METHOD(CValidate,getLastError);
PHP_METHOD(CValidate,isIp);
PHP_METHOD(CValidate,isDate);
PHP_METHOD(CValidate,isJson);
PHP_METHOD(CValidate,getLastErrorCode);

