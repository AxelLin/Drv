/******************************************************************************

  Copyright (C), 2001-2011, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : tde_debug.h
  Version       : Initial Draft
  Author        : w54130
  Created       : 2007/5/21
  Last Modified :
  Description   : 
  Function List :
  History       :
  1.Date        : 2007/5/21
    Author      : w54130
    Modification: Created file

******************************************************************************/
#ifndef __TDE_DEBUG_H__
#define __TDE_DEBUG_H__

#include "hi_type.h"
#include "tde_interface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef __KERNEL__
/******************************************************************************
 * 以下供应用层使用，内核代码不使用该定义 
 *
 ******************************************************************************
 */
#define TDE_CHECK_PTR(ptr)			    			    \
        do{                                                 \
            if(NULL == ptr)                                 \
            {                                               \
                printf("null ptr!\n");                      \
                return HI_ERR_TDE_NULL_PTR;                          \
            }                                               \
        }while(0)

#ifdef TDE_DEBUG
    #define TDE_PRINT(fmt,args...) printf ("%s, %s, line %d:" fmt, __FILE__, __FUNCTION__, __LINE__, ## args)
#else
    #define TDE_PRINT(fmt,args...)
#endif
            
/* TODO:  */

#else
/******************************************************************************
 * 以下供内核使用，应用层代码不使用该定义 
 *
 ******************************************************************************
 */

#ifdef TDE_DEBUG
    #define TDE_PRINT(fmt,args...) printk ("%s, %s, line %d:" fmt, __FILE__, __FUNCTION__, __LINE__, ## args)
#else
    #define TDE_PRINT(fmt,args...)
#endif

#define TDE_CHECK_PTR(ptr)			    			    \
	do{													\
    	if(NULL == ptr)			                        \
    	{												\
            printk(KERN_WARNING "null ptr!\n");         \
       		return HI_ERR_TDE_NULL_PTR;           	            \
    	}											 	\
    }while(0)
    
#endif

//#define _INT_SUPPORT

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* End of #ifndef __TDE_DEBUG_H__ */


