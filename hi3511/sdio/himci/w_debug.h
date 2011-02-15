/******************************************************************************
Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : hi_debug.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2005/4/23
Last Modified :
Description   : The common debug macro defination 
Function List :
History       :
******************************************************************************/
#ifndef __HI_DEBUG_H__
#define __HI_DEBUG_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#ifdef __KERNEL__
    #define W_PRINT printk
#else
    #define W_PRINT printf
#endif

/* Report the current position */
#define POS_REPORT()                                    \
    do{                                                 \
        W_PRINT("  >File     : "__FILE__"\n"           \
                 "  >Function : %s\n"                   \
                 "  >Line     : %d\n"                   \
                 ,__FUNCTION__, __LINE__);              \
    }while(0)
//#define HI_DEBUG
#ifdef HI_DEBUG

/* Using samples:   HI_ASSERT(x>y); */
#define HI_ASSERT(expr)                                     \
    do{                                                     \
        if (!(expr)) {                                      \
            W_PRINT("\nASSERT Failure{" #expr "}\n");      \
            POS_REPORT();                                   \
        }                                                   \
    }while(0)

/* Using samples: 
 * HI_TRACE(HI_INFO_LEVEL(255), HI_ID_DRV3510, "Test %d, %s\n", 12, "Test");
**/

#undef HI_TRACE
#define CUR_LEVEL_LOW 0
#define CUR_LEVEL_HIG 5
#define HI_TRACE(level,args...) \
do{\
    if ((level <= CUR_LEVEL_HIG) && (level >= CUR_LEVEL_LOW)){\
        W_PRINT("File Name: %s Function: %s Line: %i\n", \
            __FILE__,__FUNCTION__, __LINE__);\
        W_PRINT(args);\
    } \
}while(0)

#define	_ENTER()  \
do{\
      if ((1 <= CUR_LEVEL_HIG) && (1 >= CUR_LEVEL_LOW)){\
                   W_PRINT("Enter: %s, %s linux %i\n", __FUNCTION__, \
							__FILE__, __LINE__); \
       } \
}while(0)
#define	_LEAVE()  \
do{ \
      if ((1 <= CUR_LEVEL_HIG) && (1 >= CUR_LEVEL_LOW)){\
                   W_PRINT("Leave: %s, %s linux %i\n", __FUNCTION__, \
							__FILE__, __LINE__); \
       } \
}while(0)


/* Using samples: 
 * ASSERT_RETURN((x>y), HI_FAILURE); // For int return functions 
 * ASSERT_RETURN((x>y), RET_VOID);   // For void return functions
**/
#define ASSERT_RETURN(exp, err)                             \
    do{                                                     \
        if (!(exp)) {                                       \
            W_PRINT("\nASSERT Failure{" #exp "}\n");       \
            W_PRINT("  >Error    : 0x%08x\n", err);        \
            POS_REPORT();                                   \
            return err;                                     \
        }                                                   \
    }while(0)

/* Using samples:  DBG_PRINT("This is a test!\n");*/
#define DBG_PRINT(args...)                                  \
    do{                                                     \
        POS_REPORT();                                       \
        W_PRINT(args);                                     \
    }while(0)

#else /* #ifndef HI_DEBUG*/

#define HI_ASSERT(expr)
#define HI_TRACE(level, args...)
#define ASSERT_RETURN(exp, err) 
#define DBG_PRINT(args...)
#define	_ENTER()
#define	_LEAVE()

#endif /* End of #ifdef HI_DEBUG */

/* Using samples: 
 * CHECK_RETURN((pData != NULL), HI_FAILURE, "Pointer is NULL\n");
 * This macro will be used for parament check. It's useble forever.
**/
#define CHECK_RETURN(exp, err, args...)     \
    if(!(exp))                              \
    {                                       \
        DBG_PRINT(args);                    \
        return err;                         \
    }


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __HI_DEBUG_H__ */

