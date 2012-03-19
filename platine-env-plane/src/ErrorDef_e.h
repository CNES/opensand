#ifndef ErrorDef_e
#   define ErrorDef_e
/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ErrorDef class implements the error definition 
               configuration file reading
    @HISTORY :
    03-02-24 : Creation
    03-10-20 : Add XML data (GM)
    04-02-12 : P.LOPEZ : Increase max nb of defined errors.
*/
/*--------------------------------------------------------------------------*/

#   include "Error_e.h"

/*********************/
/* MACRO DEFINITIONS */
/*********************/
/* All these limits shall be reconsidered at integration-time  */
#   define C_ERR_DEF_MAX_CAR_NAME     64
													/* maximum number of characters for error Name */
#   define C_ERR_DEF_MAX_CAR_IDX_SIGN 32
													/* maximum number of characters for index signification */
#   define C_ERR_DEF_MAX_CAR_VAL_SIGN 32
													/* maximum number of characters for value signification */
#   define C_ERR_DEF_MAX_CAR_UNIT     32
													/* maximum number of characters for Unit */
#   define C_INDEX_DEF_MAX_CAR        32
													/* maximum number of characters for Index value */
#   define C_INDEX_DEF_MAX_NB         48
													/* maximum number of Index for one type */
#   define C_ERR_DEF_MAX_ERRORS       100


/********************/
/* ENUM DEFINITIONS */
/********************/
typedef enum
{
	C_ERROR_LABEL_COMMAND = 0,
	C_ERROR_LABEL_CRITICAL,
	C_ERROR_LABEL_MINOR,

	C_ERROR_LABEL_MAX_NB
} T_ERROR_LABEL;


/*************************/
/* STRUCTURE DEFINITIONS */
/*************************/
typedef T_CHAR T_INDEX_VALUE[C_INDEX_DEF_MAX_CAR];


typedef struct
{										  /* LEVEL 2 */
	T_UINT32 _nbIndex;
	T_INDEX_VALUE _IndexValues[C_INDEX_DEF_MAX_NB];
} T_INDEX_TAB;


typedef struct
{										  /* LEVEL 1 */

	T_INT32 _ErrorId;
	T_INT32 _Category;
	T_CHAR _Name[C_ERR_DEF_MAX_CAR_NAME];
	T_CHAR _IndexSignification[C_ERR_DEF_MAX_CAR_IDX_SIGN];
	T_CHAR _ValueSignification[C_ERR_DEF_MAX_CAR_VAL_SIGN];
	T_CHAR _Unit[C_ERR_DEF_MAX_CAR_UNIT];

	T_INDEX_TAB _IndexTab;
} T_ERROR_DEF;


typedef struct
{										  /* LEVEL 0 */
	T_UINT32 _nbError;
	T_ERROR_DEF _Error[C_ERR_DEF_MAX_ERRORS];
} T_ERRORS_DEF;


T_ERROR ERROR_DEF_ReadConfigFile(
											  /* INOUT */ T_ERRORS_DEF * ptr_this);


#endif /* ErrorDef_e */
