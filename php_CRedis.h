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
zend_class_entry	*CRedisCe,
					*CRedisMessageCe;


//类方法:创建应用
PHP_METHOD(CRedis,getInstance);
PHP_METHOD(CRedis,__construct);
PHP_METHOD(CRedis,__call);
PHP_METHOD(CRedis,getObject);
PHP_METHOD(CRedis,set);

PHP_METHOD(CRedisMessage,getBody);
PHP_METHOD(CRedisMessage,ack);
PHP_METHOD(CRedisMessage,__call);

int CRedis_checkWriteRead(char *config TSRMLS_DC);

