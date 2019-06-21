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

//zend�����
zend_class_entry	*CExceptionCe,
					*CPluginExceptionCe,
					*CacheExceptionCe,
					*CRedisExceptionCe,
					*CClassNotFoundExceptionCe,
					*CDbExceptionCe,
					*CModelExceptionCe,
					*CRouteExceptionCe,
					*CSessionCookieExceptionCe,
					*CViewExceptionCe,
					*CShellExceptionCe,
					*CHttpExceptionCe,
					*CQueueExceptionCe,
					*CMailExceptionCe,
					*CFtpExceptionCe,
					*CMicroServerExceptionCe;


//�෽��:����Ӧ��
PHP_METHOD(CException,__construct);
PHP_METHOD(CException,__destruct);
PHP_METHOD(CException,getTopErrors);
PHP_METHOD(CException,getTopException);
PHP_METHOD(CException,closeErrorShow);
PHP_METHOD(CException,getErrorShow);
PHP_METHOD(CException,hasFatalErrors);
PHP_METHOD(CException,filterFileTruePath);


