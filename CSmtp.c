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
#include "php_CSmtp.h"
#include "php_CException.h"
#include "ext/standard/php_smart_str_public.h"
#include "ext/standard/php_smart_str.h"


//zend�෽��
zend_function_entry CSmtp_functions[] = {
	PHP_ME(CSmtp,__construct,NULL,ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(CSmtp,send,NULL,ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

//ģ�鱻����ʱ
CMYFRAME_REGISTER_CLASS_RUN(CSmtp)
{
	zend_class_entry funCe;
	INIT_CLASS_ENTRY(funCe,"CSmtp",CSmtp_functions);
	CSmtpCe = zend_register_internal_class(&funCe TSRMLS_CC);

	//�������
	zend_declare_property_long(CSmtpCe, ZEND_STRL("smtp_port"),25,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_long(CSmtpCe, ZEND_STRL("time_out"),40,ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CSmtpCe, ZEND_STRL("host_name"),"localhost",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CSmtpCe, ZEND_STRL("log_file"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CSmtpCe, ZEND_STRL("relay_host"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CSmtpCe, ZEND_STRL("auth"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CSmtpCe, ZEND_STRL("user"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CSmtpCe, ZEND_STRL("pass"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(CSmtpCe, ZEND_STRL("sock"),ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CSmtpCe, ZEND_STRL("displayname"),"NoReplayMail",ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_string(CSmtpCe, ZEND_STRL("debug"),"",ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_string(CSmtpCe, ZEND_STRL("sendType"),"mail",ZEND_ACC_PRIVATE TSRMLS_CC);

	return SUCCESS;
}

PHP_METHOD(CSmtp,__construct){

	char	*mailHost,
			*mailUser,
			*mailPass,
			*siteName = "";

	int		mailHostLen = 0,
			mailUserLen = 0,
			mailPassLen = 0,
			debug = 0,
			siteNameLen = 0;

	long	mailPort = 25;


	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"slss|bs",&mailHost,&mailHostLen,&mailPort,&mailUser,&mailUserLen,&mailPass,&mailPassLen,&debug,&siteName,&siteNameLen) == FAILURE){
		zend_throw_exception(CMailExceptionCe, "[CMailException] __construct execution method parameter error", 12012 TSRMLS_CC);
		return;
	}

	//mailHost
	zend_update_property_string(CSmtpCe,getThis(),ZEND_STRL("relay_host"),mailHost TSRMLS_CC);

	//mailPort
	zend_update_property_long(CSmtpCe,getThis(),ZEND_STRL("smtp_port"),mailPort TSRMLS_CC);

	//mailUser
	zend_update_property_string(CSmtpCe,getThis(),ZEND_STRL("user"),mailUser TSRMLS_CC);

	//mailPass
	zend_update_property_string(CSmtpCe,getThis(),ZEND_STRL("pass"),mailPass TSRMLS_CC);

	//siteName
	if(siteNameLen > 0){
		zend_update_property_string(CSmtpCe,getThis(),ZEND_STRL("displayname"),siteName TSRMLS_CC);
	}
}

int CSmtp_putCMD(zval *object,char *command,char *args TSRMLS_DC){

	char *runCommand;
	if(strlen(args) > 0){
		spprintf(&runCommand,0,"%s%s%s%s",command," ",args,"\r\n");
	}else{
		spprintf(&runCommand,0,"%s%s",command,"\r\n");
	}


	//ִ��fwrite����
	MODULE_BEGIN
		zval	constructReturn,
				constructVal,
				*paramsList[2],
				params1,
				params2,
				*returnObject;
		paramsList[0] = &params1;
		paramsList[1] = &params2;
		MAKE_STD_ZVAL(paramsList[0]);
		MAKE_STD_ZVAL(paramsList[1]);

		ZVAL_ZVAL(paramsList[0],object,1,0);
		ZVAL_STRING(paramsList[1],runCommand,1);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"fwrite", 0);
		call_user_function(EG(function_table), NULL, &constructVal, &constructReturn, 2, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_ptr_dtor(&paramsList[1]);
		zval_dtor(&constructReturn);
	MODULE_END

	//�ж�ִ��״̬
	MODULE_BEGIN
		zval	constructReturn,
				constructVal,
				*paramsList[2],
				params1,
				params2,
				*returnObject,
				*match;
	
		char	*replaceString;
		paramsList[0] = &params1;
		paramsList[1] = &params2;
		MAKE_STD_ZVAL(paramsList[0]);
		MAKE_STD_ZVAL(paramsList[1]);
		ZVAL_ZVAL(paramsList[0],object,1,0);
		ZVAL_LONG(paramsList[1],512);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"fgets", 0);
		call_user_function(EG(function_table), NULL, &constructVal, &constructReturn, 2, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_ptr_dtor(&paramsList[1]);
		returnObject = &constructReturn;
		if(IS_STRING == Z_TYPE_P(returnObject)){
			replaceString = estrdup(Z_STRVAL_P(returnObject));
			if(!preg_match("/^[23]/i",replaceString,&match)){
				zval_ptr_dtor(&match);
				efree(replaceString);
				zval_dtor(&constructReturn);
				return 0;
			}
			efree(replaceString);
			zval_ptr_dtor(&match);
		}
		zval_dtor(&constructReturn);
	MODULE_END


	efree(runCommand);
	return 1;
}

void CSmtp_close(zval *mailSocket TSRMLS_DC){
	zval	constructVal,
			functionReturn,
			*paramsList[1],
			params1;
	paramsList[0] = &params1;
	MAKE_STD_ZVAL(paramsList[0]);
	ZVAL_ZVAL(paramsList[0],mailSocket,1,0);
	INIT_ZVAL(constructVal);
	ZVAL_STRING(&constructVal,"fclose", 0);
	call_user_function(EG(function_table), NULL, &constructVal, &functionReturn, 1, paramsList TSRMLS_CC);
	zval_ptr_dtor(&paramsList[0]);
	zval_dtor(&functionReturn);
}

PHP_METHOD(CSmtp,send){

	char	*from,
			*subject,
			*body,
			*addHeader = "",
			*mailType = "HTML",
			*cc = "",
			*bcc = "";

	smart_str sendHeader = {0};

	int		fromLen = 0,
			subjectLen = 0,
			bodyLen = 0,
			addHeaderLen = 0,
			mailTypeLen = 0,
			ccLen = 0,
			bccLen = 0,
			i,h;

	zval	*toArray,
			*relay_host,
			*smtp_port,
			*mailSocket,
			mailSocketReturn,
			*mailUser,
			*mailPass,
			*toList,
			**thisTo;
			


	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"zsss|ssss",&toArray,&from,&fromLen,&subject,&subjectLen,&body,&bodyLen,&addHeader,&addHeaderLen,&mailType,&mailTypeLen,&cc,&ccLen,&bcc,&bccLen) == FAILURE){
		zend_throw_exception(CMailExceptionCe, "[CMailException] send execution method parameter error", 12012 TSRMLS_CC);
		return;
	}

	if(fromLen <= 0 || subjectLen <= 0 || bodyLen <= 0){
		zend_throw_exception(CMailExceptionCe, "[CMailException] send execution method parameter error", 12012 TSRMLS_CC);
		return;
	}

	MAKE_STD_ZVAL(toList);
	if(IS_ARRAY == Z_TYPE_P(toArray)){
		ZVAL_ZVAL(toList,toArray,1,0);
	}else if(IS_STRING == Z_TYPE_P(toArray)){
		array_init(toList);
		add_next_index_string(toList,Z_STRVAL_P(toArray),1);
	}else{
		array_init(toList);
	}


	//ͷ
	smart_str_appends(&sendHeader,"MIME-Version:1.0\r\n");
	if(strcmp(mailType,"HTML") == 0){
		smart_str_appends(&sendHeader,"Content-Type:text/html;charset=utf-8\r\n");
	}

	//���to
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(toList));
	h = zend_hash_num_elements(Z_ARRVAL_P(toList));
	for(i = 0 ; i < h ; i++){
		zend_hash_get_current_data(Z_ARRVAL_P(toList),(void**)&thisTo);
		smart_str_appends(&sendHeader,"To: ");
		smart_str_appends(&sendHeader,Z_STRVAL_PP(thisTo));
		smart_str_appends(&sendHeader,"\r\n");
		zend_hash_move_forward(Z_ARRVAL_P(toList));
	}

	//CC
	if(ccLen > 0){
		smart_str_appends(&sendHeader,"Cc: ");
		smart_str_appends(&sendHeader,cc);
		smart_str_appends(&sendHeader,"\r\n");
	}

	//from
	MODULE_BEGIN
		zval *deisplayNameZval;
		char *showDisplayName;
		deisplayNameZval = zend_read_property(CSmtpCe,getThis(),ZEND_STRL("displayname"),0 TSRMLS_CC);
		if(Z_STRLEN_P(deisplayNameZval) > 0){
			showDisplayName = estrdup(Z_STRVAL_P(deisplayNameZval));
		}else{
			showDisplayName = estrdup(from);
		}
		smart_str_appends(&sendHeader,"From: ");
		smart_str_appends(&sendHeader,showDisplayName);
		smart_str_appends(&sendHeader,"<");
		smart_str_appends(&sendHeader,from);
		smart_str_appends(&sendHeader,">\r\n");
		efree(showDisplayName);
	MODULE_END

	//subject
	MODULE_BEGIN
		smart_str_appends(&sendHeader,"Subject: ");
		smart_str_appends(&sendHeader,subject);
		smart_str_appends(&sendHeader,"\r\n");
	MODULE_END

	//addHeader
	MODULE_BEGIN
		if(addHeaderLen >0 ){
			smart_str_appends(&sendHeader,addHeader);
			smart_str_appends(&sendHeader,"\r\n");
		}
	MODULE_END

	//date
	MODULE_BEGIN
		char *thisDate;
		php_date("r",&thisDate);
		smart_str_appends(&sendHeader,"Date: ");
		smart_str_appends(&sendHeader,thisDate);
		smart_str_appends(&sendHeader,"\r\n");
		efree(thisDate);
	MODULE_END

	//powered by
	MODULE_BEGIN
		smart_str_appends(&sendHeader,"X-Mailer: By CQuickFramework v.3.0.1\r\n");
	MODULE_END

	//message-ID
	MODULE_BEGIN
		char *timeUnix;
		char messageIdArr[1024];
		php_date("YmdHis",&timeUnix);
		sprintf(messageIdArr,"%s.%s",timeUnix,from);
		smart_str_appends(&sendHeader,"Message-ID:<");
		smart_str_appends(&sendHeader,messageIdArr);
		smart_str_appends(&sendHeader,">\r\n");
		efree(timeUnix);
	MODULE_END

	smart_str_0(&sendHeader);


	//ʹ��socket�����ʼ�
	relay_host = zend_read_property(CSmtpCe,getThis(),ZEND_STRL("relay_host"),0 TSRMLS_CC);
	smtp_port = zend_read_property(CSmtpCe,getThis(),ZEND_STRL("smtp_port"),0 TSRMLS_CC);
	mailUser = zend_read_property(CSmtpCe,getThis(),ZEND_STRL("user"),0 TSRMLS_CC);
	mailPass = zend_read_property(CSmtpCe,getThis(),ZEND_STRL("pass"),0 TSRMLS_CC);

	//��smtp���õ�socket
	MODULE_BEGIN
		zval	constructVal,
				*paramsList[5],
				params1,
				params2,
				params3,
				params4,
				params5;
		paramsList[0] = &params1;
		paramsList[1] = &params2;
		paramsList[2] = &params3;
		paramsList[3] = &params4;
		paramsList[4] = &params5;
		MAKE_STD_ZVAL(paramsList[0]);
		MAKE_STD_ZVAL(paramsList[1]);
		MAKE_STD_ZVAL(paramsList[2]);
		MAKE_STD_ZVAL(paramsList[3]);
		MAKE_STD_ZVAL(paramsList[4]);
		ZVAL_ZVAL(paramsList[0],relay_host,1,0);
		ZVAL_ZVAL(paramsList[1],smtp_port,1,0);
		ZVAL_STRING(paramsList[2],"",1);
		ZVAL_STRING(paramsList[3],"",1);
		ZVAL_LONG(paramsList[4],40);
		INIT_ZVAL(constructVal);
		ZVAL_STRING(&constructVal,"fsockopen", 0);
		call_user_function(EG(function_table), NULL, &constructVal, &mailSocketReturn, 5, paramsList TSRMLS_CC);
		zval_ptr_dtor(&paramsList[0]);
		zval_ptr_dtor(&paramsList[1]);
		zval_ptr_dtor(&paramsList[2]);
		zval_ptr_dtor(&paramsList[3]);
		zval_ptr_dtor(&paramsList[4]);
		mailSocket = &mailSocketReturn;
	MODULE_END

	if(IS_RESOURCE != Z_TYPE_P(mailSocket)){
		smart_str_free(&sendHeader);
		zval_dtor(&mailSocketReturn);
		zend_throw_exception(CMailExceptionCe, "[CMailException] can not open this smtp socket", 12012 TSRMLS_CC);
		return;
	}


	//����HELO
	if(!CSmtp_putCMD(mailSocket,"HELO","location" TSRMLS_CC)){
		CSmtp_close(mailSocket TSRMLS_CC);
		smart_str_free(&sendHeader);
		zval_dtor(&mailSocketReturn);
		zend_throw_exception(CMailExceptionCe, "[CMailException] run command Fail : HELO", 12012 TSRMLS_CC);
		return;
	}

	//AUTO login
	MODULE_BEGIN
		char *base64User,
			 *base64Pass;
		base64Encode(Z_STRVAL_P(mailUser),&base64User);
		base64Encode(Z_STRVAL_P(mailPass),&base64Pass);
		if(!CSmtp_putCMD(mailSocket,"AUTH LOGIN",base64User TSRMLS_CC)){
			efree(base64User);
			efree(base64Pass);
			smart_str_free(&sendHeader);
			CSmtp_close(mailSocket TSRMLS_CC);
			zval_dtor(&mailSocketReturn);
			zend_throw_exception(CMailExceptionCe, "[CMailException] run command Fail : auth Login", 12012 TSRMLS_CC);
			return;
		}
		if(!CSmtp_putCMD(mailSocket,base64Pass,"" TSRMLS_CC)){
			efree(base64User);
			efree(base64Pass);
			smart_str_free(&sendHeader);
			CSmtp_close(mailSocket TSRMLS_CC);
			zval_dtor(&mailSocketReturn);
			zend_throw_exception(CMailExceptionCe, "[CMailException] run command Fail : HELO", 12012 TSRMLS_CC);
			return;
		}
		efree(base64User);
		efree(base64Pass);
	MODULE_END

	//MAIL
	MODULE_BEGIN
		char *mailFrom;
		strcat2(&mailFrom,"FROM:<",from,">",NULL);
		if(!CSmtp_putCMD(mailSocket,"MAIL",mailFrom TSRMLS_CC)){
			CSmtp_close(mailSocket TSRMLS_CC);
			efree(mailFrom);
			smart_str_free(&sendHeader);
			zval_dtor(&mailSocketReturn);
			zend_throw_exception(CMailExceptionCe, "[CMailException] run command Fail : FROM", 12012 TSRMLS_CC);
			return;
		}
		efree(mailFrom);
	MODULE_END

	//TO
	MODULE_BEGIN

		zend_hash_internal_pointer_reset(Z_ARRVAL_P(toList));
		h = zend_hash_num_elements(Z_ARRVAL_P(toList));
		for(i = 0 ; i < h ; i++){
			zend_hash_get_current_data(Z_ARRVAL_P(toList),(void**)&thisTo);

			MODULE_BEGIN
				char *mailFrom;
				strcat2(&mailFrom,"TO:<",Z_STRVAL_PP(thisTo),">",NULL);
				if(!CSmtp_putCMD(mailSocket,"RCPT",mailFrom TSRMLS_CC)){
					CSmtp_close(mailSocket TSRMLS_CC);
					efree(mailFrom);
					smart_str_free(&sendHeader);
					zval_dtor(&mailSocketReturn);
					zend_throw_exception(CMailExceptionCe, "[CMailException] run command Fail : To", 12012 TSRMLS_CC);
					return;
				}
				efree(mailFrom);
			MODULE_END
			
			zend_hash_move_forward(Z_ARRVAL_P(toList));
		}
	MODULE_END
	zval_ptr_dtor(&toList);


	//Data
	if(!CSmtp_putCMD(mailSocket,"DATA","" TSRMLS_CC)){
		CSmtp_close(mailSocket TSRMLS_CC);
		smart_str_free(&sendHeader);
		zval_dtor(&mailSocketReturn);
		zend_throw_exception(CMailExceptionCe, "[CMailException] run command Fail : Data", 12012 TSRMLS_CC);
		return;
	}

	//������Ϣ
	MODULE_BEGIN
		char *message;
		spprintf(&message,0,"%s%s%s",sendHeader.c,"\r\n",body);
		if(!CSmtp_putCMD(mailSocket,message,"" TSRMLS_CC)){
			CSmtp_close(mailSocket TSRMLS_CC);
			efree(message);
			smart_str_free(&sendHeader);
			zval_dtor(&mailSocketReturn);
			zend_throw_exception(CMailExceptionCe, "[CMailException] run command Fail : Message", 12012 TSRMLS_CC);
			return;
		}
		efree(message);
	MODULE_END


	if(!CSmtp_putCMD(mailSocket,"\r\n.","" TSRMLS_CC)){
		CSmtp_close(mailSocket TSRMLS_CC);
		smart_str_free(&sendHeader);
		zval_dtor(&mailSocketReturn);
		zend_throw_exception(CMailExceptionCe, "[CMailException] run command Fail : eom", 12012 TSRMLS_CC);
		return;
	}

	if(!CSmtp_putCMD(mailSocket,"QUIT","" TSRMLS_CC)){
		CSmtp_close(mailSocket TSRMLS_CC);
		smart_str_free(&sendHeader);
		zval_dtor(&mailSocketReturn);
		zend_throw_exception(CMailExceptionCe, "[CMailException] run command Fail : Quit", 12012 TSRMLS_CC);
		return;
	}


	//�ͷ���Դ
	CSmtp_close(mailSocket TSRMLS_CC);
	smart_str_free(&sendHeader);
	zval_dtor(&mailSocketReturn);
}