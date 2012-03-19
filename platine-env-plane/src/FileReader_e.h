#ifndef FileReader_e
#   define FileReader_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The FileReader class implements methods to read 
               formatted data in configuration files. 
    @HISTORY :
    03-02-27 : Creation
    03-05-06 : Replace DOUBLE by FLOAT (PLo).
*/
/*--------------------------------------------------------------------------*/

#   include <stdio.h>
#   include "Error_e.h"
#   include "LineParser_e.h"


typedef struct
{

	FILE *_File;

	T_LINE_PARSER _Parser;

	T_BYTE _Buffer[C_FR_MAX_LINE];

} T_FILE_READER;

typedef struct
{
	T_UINT32 _nbItem;
	T_BYTE _items[];

} T_ITEM_TAB;



/******************************/
/* Only static variables used */
/* Init & terminate methods    */
T_ERROR FILE_READER_Init(
									/* INOUT */ T_FILE_READER * ptr_this);


/* All item-reading functions shall respect the following prototype,
    in order to use the ReadLoop() and ReadNamedLoop() methods */
typedef T_ERROR(*T_READ_ITEM_FUNC) (
												  /* IN    */ T_FILE_READER * ptr_reader,
												  /*   OUT */ T_ITEM_TAB * ptr_config,
												  /* IN    */ T_UINT32 itemIndex);


T_ERROR FILE_READER_OpenFile(
										 /* IN    */ T_FILE_READER * ptr_this,
										 /* IN    */ T_STRING file_name);

T_ERROR FILE_READER_CloseFile(
										  /* IN    */ T_FILE_READER * ptr_this);

T_ERROR FILE_READER_OpenBlock(
										  /* IN    */ T_FILE_READER * ptr_this);

T_ERROR FILE_READER_CloseBlock(
											/* IN    */ T_FILE_READER * ptr_this);

T_ERROR FILE_READER_ReadLine(
										 /* IN    */ T_FILE_READER * ptr_this,
										 /*   OUT */ T_STRING ptr_value);

T_ERROR FILE_READER_ReadName(
										 /* IN    */ T_FILE_READER * ptr_this,
										 /* IN    */ T_STRING name);

T_ERROR FILE_READER_ReadNamedString(
												  /* IN    */ T_FILE_READER * ptr_this,
												  /* IN    */ T_STRING name,
												  /* IN    */ T_UINT32 max_len,
												  /*   OUT */ T_STRING ptr_value);

T_ERROR FILE_READER_ReadNamedInteger(
													/* IN    */ T_FILE_READER * ptr_this,
													/* IN    */ T_STRING name,
													/* IN    */ T_INT32 min_value,
													/* IN    */ T_INT32 max_value,
													/*   OUT */ T_INT32 * ptr_value);

T_ERROR FILE_READER_ReadNamedIntegerDefault(
															 /* IN    */ T_FILE_READER *
															 ptr_this,
															 /* IN    */ T_STRING name,
															 /* IN    */ T_INT32 min_value,
															 /* IN    */ T_INT32 max_value,
															 /* IN    */ T_INT32 default_value,
															 /*   OUT */ T_INT32 * ptr_value);

T_ERROR FILE_READER_ReadNamedUInteger(
													 /* IN    */ T_FILE_READER * ptr_this,
													 /* IN    */ T_STRING name,
													 /* IN    */ T_UINT32 min_value,
													 /* IN    */ T_UINT32 max_value,
													 /*   OUT */ T_UINT32 * ptr_value);

T_ERROR FILE_READER_ReadNamedUIntegerDefault(
															  /* IN    */ T_FILE_READER *
															  ptr_this,
															  /* IN    */ T_STRING name,
															  /* IN    */ T_UINT32 min_value,
															  /* IN    */ T_UINT32 max_value,
															  /* IN    */
															  T_UINT32 default_value,
															  /*   OUT */
															  T_UINT32 * ptr_value);

T_ERROR FILE_READER_ReadNamedEnum(
												/* IN    */ T_FILE_READER * ptr_this,
												/* IN    */ T_STRING name,
												/* IN    */ T_ENUM_COUPLE choices[],
												/*   OUT */ T_INT32 * ptr_value);

T_ERROR FILE_READER_ReadNamedFloat(
												 /* IN    */ T_FILE_READER * ptr_this,
												 /* IN    */ T_STRING name,
												 /* IN    */ T_FLOAT min_value,
												 /* IN    */ T_FLOAT max_value,
												 /*   OUT */ T_FLOAT * ptr_value);

T_ERROR FILE_READER_ReadNamedFloatDefault(
														  /* IN    */ T_FILE_READER *
														  ptr_this,
														  /* IN    */ T_STRING name,
														  /* IN    */ T_FLOAT min_value,
														  /* IN    */ T_FLOAT max_value,
														  /* IN    */ T_FLOAT default_value,
														  /*   OUT */ T_FLOAT * ptr_value);

T_ERROR FILE_READER_ReadLoop(
										 /* IN    */ T_FILE_READER * ptr_this,
										 /* IN    */ T_READ_ITEM_FUNC read_item_func,
										 /* IN    */ T_UINT32 nb_loop,
										 /*   OUT */ T_ITEM_TAB * ptr_config);

T_ERROR FILE_READER_ReadNamedLoop(
												/* IN    */ T_FILE_READER * ptr_this,
												/* IN    */ T_STRING loop_name,
												/* IN    */ T_READ_ITEM_FUNC read_item_func,
												/* IN    */ T_UINT32 max_loop,
												/*   OUT */ T_ITEM_TAB * ptr_config);

#endif
