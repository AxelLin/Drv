#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "hi_comm_video.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "pciv_test_comm.h"
#include "pci_trans.h"

#include "hi_comm_vo.h"
#include "mpi_vo.h"
#include "hi_comm_vi.h"
#include "mpi_vi.h"
#include "tw2815.h"
#include "hi_mcc_usrdev.h"
#include "pciv_msg.h"

/* We would like use the port num that less than 100 */
static int g_nMsgFd[HI_MCC_TARGET_ID_NR][PCIV_MSGPORT_MAXPORT];

/* Create message port */
HI_S32 PCIV_OpenMsgPort(HI_S32 s32TgtId)
{
    struct hi_mcc_handle_attr attr;
    HI_S32 s32Ret = HI_SUCCESS, i;
    HI_S32 s32Port[3][2] = 
        {
            {PCIV_MSGPORT_USERCMD,         0},
            {PCIV_MSGPORT_USERNOTIFY2HOST, 0},
            {PCIV_MSGPORT_USERNOTIFY2SLAVE,0}
        };

    /* Open the command port */
    for(i=0; i<3; i++)
    {
        attr.target_id = s32TgtId;
        attr.port      = s32Port[i][0];
        attr.priority  = s32Port[i][1];
        g_nMsgFd[s32TgtId][s32Port[i][0]] = open("/dev/misc/mcc_userdev", O_RDWR);
        s32Ret = ioctl(g_nMsgFd[s32TgtId][s32Port[i][0]], HI_MCC_IOC_CONNECT, &attr);
        HI_ASSERT((HI_SUCCESS == s32Ret));
    }

    return s32Ret;
}

HI_S32 PCIV_SendMsg(HI_S32 s32TgtId, HI_S32 s32Port, PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret = HI_SUCCESS, s32TimeOut=0;
    
    if(g_nMsgFd[s32TgtId][s32Port] <= 0) return HI_FAILURE;

    s32Ret = write(g_nMsgFd[s32TgtId][s32Port], pMsg, pMsg->u32MsgLen + PCIV_MSG_HEADLEN);
    while(s32Ret < 0)
    {
        if(s32TimeOut > 100) break;
        
        usleep(0);
        s32Ret = write(g_nMsgFd[s32TgtId][s32Port], pMsg, pMsg->u32MsgLen + PCIV_MSG_HEADLEN);
    }
    
    return s32Ret;
}

HI_S32 PCIV_ReadMsg(HI_S32 s32TgtId, HI_S32 s32Port, PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret = HI_SUCCESS;
    fd_set fds;
    
    if(g_nMsgFd[s32TgtId][s32Port] <= 0) return HI_FAILURE;

    FD_ZERO(&fds);
    FD_SET(g_nMsgFd[s32TgtId][s32Port], &fds);
    select(g_nMsgFd[s32TgtId][s32Port] + 1, &fds, NULL, NULL, NULL);
    s32Ret = read(g_nMsgFd[s32TgtId][s32Port], pMsg, PCIV_MSG_MAXLEN+PCIV_MSG_HEADLEN);


    if(s32Ret > PCIV_MSG_HEADLEN)
    {
        pMsg->u32MsgLen = s32Ret - PCIV_MSG_HEADLEN;
        return HI_SUCCESS;
    }
    else if(s32Ret > 0)
    {
        printf("Unknown Message!\n");
    }
    else
    {
        printf("Read Message Error!\n");
    }

    return HI_FAILURE;
}


