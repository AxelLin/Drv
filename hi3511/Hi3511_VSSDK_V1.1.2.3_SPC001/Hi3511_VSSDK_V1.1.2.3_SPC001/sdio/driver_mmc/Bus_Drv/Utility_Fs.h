
/*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *          28 May 2002
 *
 * NOTE:  THIS IS A MODIFIED VERSION OF THE ORIGINAL FOR THIS APPLICATION
 */

/**************************************************************************
 * Utility functions
 **************************************************************************/

#ifndef _UTILITY_FS_H_
#define _UTILITY_FS_H_


#define   SDIO_COM_CRC_ERROR                0x080  //bit7
#define   SDIO_ILLEGAL_CMD_ERROR            0x040  //bit6
#define   SDIO_UKNOWN_ERROR                 0x008  //bit3
#define   SDIO_INVALID_FUNCTION_NO_ERROR    0x002  //bit1
#define   SDIO_OUT_OF_RANGE_ERROR           0x001  //bit0 

/* Standard MMC commands (3.1)           type  argument     response */
   /* class 1 */
#define	CMD0    0   /* MMC_GO_IDLE_STATE        bc                    */
#define CMD1    1   /* MMC_SEND_OP_COND         bcr  [31:0]  OCR  R3  */
#define CMD2    2   /* MMC_ALL_SEND_CID         bcr               R2  */
#define CMD3    3   /* MMC_SET_RELATIVE_ADDR    ac   [31:16] RCA  R1  */
#define CMD4    4   /* MMC_SET_DSR              bc   [31:16] RCA      */

#define CMD5    5   /* SDIO_SEND_OCR            ??   ??               */

#define CMD6    6   /* HSMMC_SWITCH             ac                R1  */
                    /* For ACMD6:SET_BUS_WIDTH  ??   ??               */

#define CMD7    7   /* MMC_SELECT_CARD          ac   [31:16] RCA  R1  */
#define CMD8    8   /* HSMMC_SEND_EXT_CSD       adtc [31:16] RCA  R1  */
#define CMD9    9   /* MMC_SEND_CSD             ac   [31:16] RCA  R2  */
#define CMD10   10  /* MMC_SEND_CID             ac   [31:16] RCA  R2  */
#define CMD11   11  /* MMC_READ_DAT_UNTIL_STOP  adtc [31:0]  dadr R1  */
#define CMD12   12  /* MMC_STOP_TRANSMISSION    ac                R1b */
#define CMD13 	13  /* MMC_SEND_STATUS          ac   [31:16] RCA  R1  */
#define ACMD13 	13  /* SD_STATUS                ac   [31:2] Stuff,
                                                     [1:0]Buswidth  R1*/
#define CMD14   14  /* HSMMC_BUS_TESTING        adtc [31:16] stuff R1 */
#define CMD15   15  /* MMC_GO_INACTIVE_STATE    ac   [31:16] RCA  */
#define CMD19   19  /* HSMMC_BUS_TESTING        adtc [31:16] stuff R1 */

  /* class 2 */
#define CMD16   16  /* MMC_SET_BLOCKLEN         ac   [31:0] blkln R1  */
#define CMD17   17  /* MMC_READ_SINGLE_BLOCK    adtc [31:0] dtadd R1  */
#define CMD18   18  /* MMC_READ_MULTIPLE_BLOCK  adtc [31:0] dtadd R1  */

  /* class 3 */
#define CMD20   20  /* MMC_WRITE_DAT_UNTIL_STOP adtc [31:0] dtadd R1  */

  /* class 4 */
#define CMD23   23  /* MMC_SET_BLOCK_COUNT      adtc [31:0] dtadd R1  */
#define CMD24   24  /* MMC_WRITE_BLOCK          adtc [31:0] dtadd R1  */
#define CMD25   25  /* MMC_WRITE_MULTIPLE_BLOCK adtc              R1  */
#define CMD26   26  /* MMC_PROGRAM_CID          adtc              R1  */
#define CMD27   27  /* MMC_PROGRAM_CSD          adtc              R1  */

  /* class 6 */
#define CMD28   28  /* MMC_SET_WRITE_PROT       ac   [31:0] dtadd R1b */
#define CMD29   29  /* _CLR_WRITE_PROT          ac   [31:0] dtadd R1b */
#define CMD30   30  /* MMC_SEND_WRITE_PROT      adtc [31:0] wpdtaddr R1  */

  /* class 5 */
#define CMD32   32  /* SD_ERASE_GROUP_START    ac   [31:0] dtadd  R1  */
#define CMD33   33  /* SD_ERASE_GROUP_END      ac   [31:0] dtaddr R1  */

#define CMD35   35  /* MMC_ERASE_GROUP_START    ac   [31:0] dtadd  R1  */
#define CMD36   36  /* MMC_ERASE_GROUP_END      ac   [31:0] dtaddr R1  */
#define CMD38   38  /* MMC_ERASE                ac                 R1b */

  /* class 9 */
#define CMD39   39  /* MMC_FAST_IO              ac   <Complex>     R4  */
#define CMD40   40  /* MMC_GO_IRQ_STATE         bcr                R5  */

#define ACMD41  41  /* SD_SEND_OP_COND          ??                 R1  */

  /* class 7 */
#define CMD42   42  /* MMC_LOCK_UNLOCK          adtc               R1b */

#define ACMD51  51  /* SEND_SCR                 adtc               R1  */

#define CMD52   52  /* SDIO_RW_DIRECT           ??                 R5  */
#define CMD53   53  /* SDIO_RW_EXTENDED         ??                 R5  */

  /* class 8 */
#define CMD55   55  /* MMC_APP_CMD              ac   [31:16] RCA   R1  */
#define CMD56   56  /* MMC_GEN_CMD              adtc [0] RD/WR     R1b */

#define SDIO_RESET  100  //To differentiate CMD52 for IORESET and 
                         //other rd/wrs.
#define SDIO_ABORT  101  //To differentiate CMD52 for IO ABORT and 
                         //other rd/wrs.

/* CSD field definitions */
 
#define CSD_STRUCT_VER_1_0  0           /* Valid for system specification 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1           /* Valid for system specification 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2           /* Valid for system specification 3.1       */

#define CSD_SPEC_VER_0      0           /* Implements system specification 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1           /* Implements system specification 1.4 */
#define CSD_SPEC_VER_2      2           /* Implements system specification 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3           /* Implements system specification 3.1 */

//
enum card_state {
	CARD_STATE_EMPTY = -1,
	CARD_STATE_IDLE	 = 0,
	CARD_STATE_READY = 1,
	CARD_STATE_IDENT = 2,
	CARD_STATE_STBY	 = 3,
	CARD_STATE_TRAN	 = 4,
	CARD_STATE_DATA	 = 5,
	CARD_STATE_RCV	 = 6,
	CARD_STATE_PRG	 = 7,
	CARD_STATE_DIS	 = 8,
};

/* Error codes */
enum mmc_result_t {
	MMC_NO_RESPONSE        = -1,
	MMC_NO_ERROR           = 0,
	MMC_ERROR_OUT_OF_RANGE = 200,
	MMC_ERROR_ADDRESS,
	MMC_ERROR_BLOCK_LEN,
	MMC_ERROR_ERASE_SEQ,
	MMC_ERROR_ERASE_PARAM,
	MMC_ERROR_WP_VIOLATION,
	//MMC_ERROR_CARD_IS_LOCKED, //Status bit.No need to check.
	MMC_ERROR_LOCK_UNLOCK_FAILED,
	//MMC_ERROR_COM_CRC,        //If required,user will read this by CMD13. 
	//MMC_ERROR_ILLEGAL_COMMAND,//If required,user will read this by CMD13. 
	MMC_ERROR_CARD_ECC_FAILED,
	MMC_ERROR_CC,
	MMC_ERROR_GENERAL,
	MMC_ERROR_UNDERRUN,
	MMC_ERROR_OVERRUN,
	MMC_ERROR_CID_CSD_OVERWRITE,
        //MMC_ERROR_WP_ERASE_SKIP, //Status bit.Check during erase comamnd only.
        //MMC_ERROR_CARD_ECC_DISABLED,
	//MMC_ERROR_ERASE_RESET,   //Status bit.Check during erase comamnd only.
	MMC_ERROR_STATE_MISMATCH,
	MMC_ERROR_HEADER_MISMATCH,
	MMC_ERROR_TIMEOUT,
	MMC_ERROR_CRC,
	MMC_ERROR_DRIVER_FAILURE,
};

/*
  MMC status in R1
  Type
  	e : error bit
	s : status bit
	r : detected and set for the actual command response
	x : detected and set during command execution. the host must poll
            the card by sending status command in order to read these bits.
  Clear condition
  	a : according to the card state
	b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
	c : clear by read
 */

#define R1_OUT_OF_RANGE		(1 << 31)	/* er, c */
#define R1_ADDRESS_ERROR	(1 << 30)	/* erx, c */
#define R1_BLOCK_LEN_ERROR	(1 << 29)	/* er, c */
#define R1_ERASE_SEQ_ERROR      (1 << 28)	/* er, c */
#define R1_ERASE_PARAM		(1 << 27)	/* ex, c */
#define R1_WP_VIOLATION		(1 << 26)	/* erx, c */
#define R1_CARD_IS_LOCKED	(1 << 25)	/* sx, a */
#define R1_LOCK_UNLOCK_FAILED	(1 << 24)	/* erx, c */
#define R1_COM_CRC_ERROR	(1 << 23)	/* er, b */
#define R1_ILLEGAL_COMMAND	(1 << 22)	/* er, b */
#define R1_CARD_ECC_FAILED	(1 << 21)	/* ex, c */
#define R1_CC_ERROR		(1 << 20)	/* erx, c */
#define R1_ERROR		(1 << 19)	/* erx, c */
#define R1_UNDERRUN		(1 << 18)	/* ex, c */
#define R1_OVERRUN		(1 << 17)	/* ex, c */
#define R1_CID_CSD_OVERWRITE	(1 << 16)	/* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP	(1 << 15)	/* sx, c */
#define R1_CARD_ECC_DISABLED	(1 << 14)	/* sx, a */
#define R1_ERASE_RESET		(1 << 13)	/* sr, c */
#define R1_STATUS(x)            (x & 0xFFFFE000)
#define R1_CURRENT_STATE(x)    	((x & 0x00001E00) >> 9)	/* sx, b (4 bits) */
#define R1_READY_FOR_DATA	(1 << 8)	/* sx, a */
#define R1_SWITCH_ERROR		(1 << 7)	/* sr, c */
#define R1_APP_CMD		(1 << 5)	/* sr, c */


/* These are unpacked versions of the actual responses */

struct mmc_response_r1 {
	u8  cmd;
	u32 status;
};

struct mmc_response_r3 {  
	u32 ocr;
}; 

//=========================================================================
#define PARSE_U32(_buf,_index) (((u32)_buf[_index]) << 24) | (((u32)_buf[_index+1]) << 16) | (((u32)_buf[_index+2]) << 8) | ((u32)_buf[_index+3]);

#define PARSE_U16(_buf,_index) 	(((u16)_buf[_index]) << 8) | ((u16)_buf[_index+1]);

//=========================================================================
/*int mmc_unpack_csd( BYTE *pbResponse, struct mmc_csd *ptcsd)
  Response is long response (R2,136 bits). 

  *Parameters:
	     pbResponse:(i/p):Response buffer.
         ptcsd :(o/p):CSD register structure to hold CSD 
		              contents.
         fSDcard:(i/p): 1= SD card  0=MMC card.
  *Called from:

  *Flow:
*/
int unpack_csd( unsigned char *pbResponse, CSDreg *ptcsd, unsigned char fSDcard)
{
	u8 *buf = pbResponse;	
	//Decode CSD data received in response bits 1 to 127.
	ptcsd->Fields.csd_structure      = (buf[15] & 0xc0) >> 6;      
	ptcsd->Fields.spec_vers          = (buf[15] & 0x3c) >> 2;
	ptcsd->Fields.taac               = buf[14];
	ptcsd->Fields.nsac               = buf[13];
	ptcsd->Fields.tran_speed         = buf[12];
	ptcsd->Fields.ccc                = (*((WORD*)(&buf[10])) & 0xfff0 ) >> 4;
	ptcsd->Fields.read_bl_len        = buf[10] & 0x0f;
	ptcsd->Fields.read_bl_partial    = (buf[9] & 0x80) ? 1 : 0;
	ptcsd->Fields.write_blk_misalign = (buf[9] & 0x40) ? 1 : 0;
	ptcsd->Fields.read_blk_misalign  = (buf[9] & 0x20) ? 1 : 0;
	ptcsd->Fields.dsr_imp            = (buf[9] & 0x10) ? 1 : 0;
      if(!ptcsd->Fields.csd_structure){
		ptcsd->Fields.c_size             = ((*((WORD*)(&buf[8])) & 0x3ff ) << 2) |
			                               ((WORD)(buf[7] & 0xc0) >> 6);

		ptcsd->Fields.vdd_r_curr_min     = (buf[7] & 0x38) >> 3;

		ptcsd->Fields.vdd_r_curr_max     = buf[7] & 0x07;
		ptcsd->Fields.vdd_w_curr_min     = (buf[6] & 0xe0) >> 5; 
		ptcsd->Fields.vdd_w_curr_max     = (buf[6] & 0x1c) >> 2;
		ptcsd->Fields.c_size_mult        = ((buf[6] & 0x03) << 1) |
			                      ((buf[5] & 0x80) >> 7);
      	}else
      	{
  		ptcsd->Fields.c_size             = 	*((unsigned int*)(&buf[6]))  & 0x03fffff;
      	}
    //bits 46-32 are different for SD and MMC cards.
    if(fSDcard)
    {
      ptcsd->Fields.erase.v22.erase_grp_size = (buf[5] & 0x40) >> 6;
      ptcsd->Fields.erase.v22.sector_size  = ((buf[5] & 0x3f) << 1)|
                                                 ((buf[4] & 0x80) >> 7);
      ptcsd->Fields.wp_grp_size  = buf[4] & 0x7f;
    }    
    else //MMC card
    {
      switch ( ptcsd->Fields.csd_structure ) {
        case CSD_STRUCT_VER_1_0:
        case CSD_STRUCT_VER_1_1:
          ptcsd->Fields.erase.v22.sector_size    = (buf[5] & 0x7c) >> 2;
          ptcsd->Fields.erase.v22.erase_grp_size = ((buf[5] & 0x03) << 3)|
                                                   ((buf[4] & 0xe0) >> 5);
          break;
        case CSD_STRUCT_VER_1_2:
        default:
          ptcsd->Fields.erase.v31.erase_grp_size = (buf[5] & 0x7c) >> 2;
          ptcsd->Fields.erase.v31.erase_grp_mult = ((buf[5] & 0x03) << 3)|
                                                   ((buf[4] & 0xe0) >> 5);
          break;
      }
      ptcsd->Fields.wp_grp_size   = buf[4] & 0x1f;
    }//end of if sd card.

	ptcsd->Fields.wp_grp_enable      = (buf[3] & 0x80) ? 1 : 0;
	ptcsd->Fields.default_ecc        = (buf[3] & 0x60) >> 5;
	ptcsd->Fields.r2w_factor         = (buf[3] & 0x1c) >> 2;
	ptcsd->Fields.write_bl_len       = ((buf[3] & 0x03) << 2) |
		                      ((buf[2] & 0xc0) >> 6);
	ptcsd->Fields.write_bl_partial   = (buf[2] & 0x20) ? 1 : 0;
	ptcsd->Fields.file_format_grp    = (buf[1] & 0x80) ? 1 : 0;
	ptcsd->Fields.copy               = (buf[1] & 0x40) ? 1 : 0;
	ptcsd->Fields.perm_write_protect = (buf[1] & 0x20) ? 1 : 0;
	ptcsd->Fields.tmp_write_protect  = (buf[1] & 0x10) ? 1 : 0;
	ptcsd->Fields.file_format        = (buf[1] & 0x0c) >> 2;
	ptcsd->Fields.ecc                = buf[1] & 0x03;

	/*DEBUG(2,"  csd_structure=%d  spec_vers=%d  taac=%02x  nsac=%02x  tran_speed=%02x\n"
	      "  ccc=%04x  read_bl_len=%d  read_bl_partial=%d  write_blk_misalign=%d\n"
	      "  read_blk_misalign=%d  dsr_imp=%d  c_size=%d  vdd_r_curr_min=%d\n"
	      "  vdd_r_curr_max=%d  vdd_w_curr_min=%d  vdd_w_curr_max=%d  c_size_mult=%d\n"
	      "  wp_grp_size=%d  wp_grp_enable=%d  default_ecc=%d  r2w_factor=%d\n"
	      "  write_bl_len=%d  write_bl_partial=%d  file_format_grp=%d  copy=%d\n"
	      "  perm_write_protect=%d  tmp_write_protect=%d  file_format=%d  ecc=%d\n",
	      csd->csd_structure, csd->spec_vers, 
	      csd->taac, csd->nsac, csd->tran_speed,
	      csd->ccc, csd->read_bl_len, 
	      csd->read_bl_partial, csd->write_blk_misalign,
	      csd->read_blk_misalign, csd->dsr_imp, 
	      csd->c_size, csd->vdd_r_curr_min,
	      csd->vdd_r_curr_max, csd->vdd_w_curr_min, 
	      csd->vdd_w_curr_max, csd->c_size_mult,
	      csd->wp_grp_size, csd->wp_grp_enable,
	      csd->default_ecc, csd->r2w_factor, 
	      csd->write_bl_len, csd->write_bl_partial,
	      csd->file_format_grp, csd->copy, 
	      csd->perm_write_protect, csd->tmp_write_protect,
	      csd->file_format, csd->ecc);*/
	switch (ptcsd->Fields.csd_structure) {
	case CSD_STRUCT_VER_1_0:
	case CSD_STRUCT_VER_1_1:
		/*DEBUG(2," V22 sector_size=%d erase_grp_size=%d\n", 
		      csd->erase.v22.sector_size, 
		      csd->erase.v22.erase_grp_size);*/
		break;
	case CSD_STRUCT_VER_1_2:
	default:
		/*DEBUG(2," V31 erase_grp_size=%d erase_grp_mult=%d\n", 
		      csd->erase.v31.erase_grp_size,
		      csd->erase.v31.erase_grp_mult);*/
		break;
		
	}

	return 0;
}
//=========================================================================
//Called by Validate_Response(),as per command index,response 
//type is decided.
/*int mmc_unpack_r1( BYTE *pbResponseBuff,
                     enum card_state state )
  
  *Parameters:
	     pbResponseBuff:(i/p):Response buffer.
         state:(i/p):Enumerated type holding state of card,
		          before execution of command.
				  ==0x0ff =>Donot compare CURRENT_STATE of card.
  *Called from:

  *Flow:
        -
*/
int mmc_unpack_r1(BYTE *pbResponseBuff,
				  enum card_state state )
{
	DWORD dwCardStatus=00;

	dwCardStatus = *((DWORD*)(pbResponseBuff));

	//DEBUG(2,"status=%08x\n", dwCardStatus);

	//From card status,check for possible error bits.
	if (R1_STATUS(dwCardStatus)) {
		if ( dwCardStatus & R1_OUT_OF_RANGE )       return MMC_ERROR_OUT_OF_RANGE;
		if ( dwCardStatus & R1_ADDRESS_ERROR )      return MMC_ERROR_ADDRESS;
		if ( dwCardStatus & R1_BLOCK_LEN_ERROR )    return MMC_ERROR_BLOCK_LEN;
		if ( dwCardStatus & R1_ERASE_SEQ_ERROR )    return MMC_ERROR_ERASE_SEQ;
		if ( dwCardStatus & R1_ERASE_PARAM )        return MMC_ERROR_ERASE_PARAM;
		if ( dwCardStatus & R1_WP_VIOLATION )       return MMC_ERROR_WP_VIOLATION;
               //Status bit.No need to check.Error reflected in Lock_Unlock_failed.
	       //if ( dwCardStatus & R1_CARD_IS_LOCKED )     return MMC_ERROR_CARD_IS_LOCKED;
		if ( dwCardStatus & R1_LOCK_UNLOCK_FAILED ) return MMC_ERROR_LOCK_UNLOCK_FAILED;
		//Error of previous command.If required user will read using CMD13.
                //if ( dwCardStatus & R1_COM_CRC_ERROR )      return MMC_ERROR_COM_CRC;
		//if ( dwCardStatus & R1_ILLEGAL_COMMAND )    return MMC_ERROR_ILLEGAL_COMMAND;
		if ( dwCardStatus & R1_CARD_ECC_FAILED )    return MMC_ERROR_CARD_ECC_FAILED;
		if ( dwCardStatus & R1_CC_ERROR )           return MMC_ERROR_CC;
		if ( dwCardStatus & R1_ERROR )              return MMC_ERROR_GENERAL;
		if ( dwCardStatus & R1_UNDERRUN )           return MMC_ERROR_UNDERRUN;
		if ( dwCardStatus & R1_OVERRUN )            return MMC_ERROR_OVERRUN;
		if ( dwCardStatus & R1_CID_CSD_OVERWRITE )  return MMC_ERROR_CID_CSD_OVERWRITE;
		//Status bits.To be checked for erase commands.
	}

    //Check if state of card when command is received matches 
	//with what should be.This can be used to confirm that no state is 
	//changed.
	if(state != 0x0ff) //Donot match states,if state code is 0x0ff.
	  if( R1_CURRENT_STATE(dwCardStatus) != (unsigned int)state ) 
		return Err_State_Mismatch;

	return 0;
}

//==============================================================
/*int mmc_unpack_cid( BYTE *pbResponse, struct mmc_cid *ptcid)
  Response is long response (R2,136 bits). 

  *Parameters:
	     pbResponseBuff:(i/p):Response buffer.
         ptcid :(o/p):CID register structure to hold CID 
		              contents.
  *Called from:

  *Flow:
*/
int mmc_unpack_cid( BYTE *pbResponse, CIDreg *ptcid)
{
	u8 *buf = pbResponse;
	int i;

	//Decode bits from response and assign to CID strcuture.
	//Response is received in proper format.No need to reverse
	//byte order as it is already done in RTL.
	ptcid->mid = buf[15];
	ptcid->oid = *((WORD*)(&buf[13])); //PARSE_U16(buf,2);
	for ( i = 0 ; i < 6 ; i++ )
		ptcid->pnm[i] = buf[7+i];
	ptcid->pnm[6] = 0;
	ptcid->prv = buf[6];
	ptcid->psn = *((DWORD*)(&buf[2])); //PARSE_U32(buf,11);
	ptcid->mdt = buf[1];
	
	/*DEBUG(2," mid=%d oid=%d pnm=%s prv=%d.%d psn=%08x mdt=%d/%d\n",
	      cid->mid, cid->oid, cid->pnm, 
	      (cid->prv>>4), (cid->prv&0xf), 
	      cid->psn, (cid->mdt>>4), (cid->mdt&0xf)+1997);*/

   	return 0;
}
//==============================================================
int Unpack_CMD52_Resp(BYTE* pbRespData)
{
	BYTE bRespStatus;

	bRespStatus = *(pbRespData+1);

	if (bRespStatus) {
        //Response to CMD42 is R1.For COM_CRC and Illegal command errors,
        //response timeout will occur. No need to read status again.
		if(bRespStatus & SDIO_UKNOWN_ERROR )    
		   return Err_SDIO_Resp_UKNOWN;
		if(bRespStatus & SDIO_INVALID_FUNCTION_NO_ERROR )    
		   return Err_SDIO_Resp_INVALID_FUNCTION_NO;
		if(bRespStatus & SDIO_OUT_OF_RANGE_ERROR )
		   return Err_SDIO_Resp_OUT_OF_RANGE;
	}

	return SUCCESS;
}
//==============================================================
void Unpack_CCCR_reg(int nCardNo, BYTE *pbResponse)
{
   Card_Info[nCardNo].CCCR.CCCR_Revision  = *pbResponse;
   Card_Info[nCardNo].CCCR.Specs_Revision = *(pbResponse+1);
   Card_Info[nCardNo].CCCR.IO_Enable      = *(pbResponse+2);
   Card_Info[nCardNo].CCCR.Int_Enable     = *(pbResponse+4);
   Card_Info[nCardNo].CCCR.Bus_Interface  = *(pbResponse+7);
   Card_Info[nCardNo].CCCR.Card_Capability= *(pbResponse+8);
   Card_Info[nCardNo].CCCR.Common_CIS_ptr = 
	                   *((DWORD*)(pbResponse+9)) & 0x0ffffff;
   Card_Info[nCardNo].CCCR.Power_Control  = *(pbResponse+0x12);
}

//==============================================================

/*int Repack_csd( BYTE *pbResponse, struct mmc_csd *ptcsd)
  Repacks the CSD data ie from CSD structure,bit data is filled in
  128 bit data.

  *Parameters:
	     pbResponse:(i/p):Response buffer.
         ptcsd :(o/p):CSD register structure to hold CSD 
		              contents.
         fSDcard:(i/p): 1= SD card  0=MMC card.
  *Called from:

  *Flow:
*/
void Repack_csd( BYTE *pbResponse, CSDreg *ptcsd, unsigned char fSDcard)
{
	u8 *buf = pbResponse;

   //Encode CSD data received in response bits 1 to 127.
   buf[15] = (ptcsd->Fields.csd_structure << 6) | 
             (ptcsd->Fields.spec_vers << 2);
   buf[14] = ptcsd->Fields.taac;
   buf[13] = ptcsd->Fields.nsac ;
   buf[12] = ptcsd->Fields.tran_speed;
   buf[11] = (ptcsd->Fields.ccc & 0xfff) >> 4;
   buf[10] = ((ptcsd->Fields.ccc & 0xf) << 4) |
             ptcsd->Fields.read_bl_len;

   buf[9]  = (ptcsd->Fields.read_bl_partial << 7) |
             (ptcsd->Fields.write_blk_misalign << 6) |
             (ptcsd->Fields.read_blk_misalign << 5) |
             (ptcsd->Fields.dsr_imp << 4) |
             ((ptcsd->Fields.c_size & 0xc00) >> 10);   
 
   buf[8]  = ((ptcsd->Fields.c_size & 0x3fc) >> 2);       

   buf[7]  = ((ptcsd->Fields.c_size & 0x3) <<6) |
             (ptcsd->Fields.vdd_r_curr_min << 3) |
             ptcsd->Fields.vdd_r_curr_max;

   buf[6]  = (ptcsd->Fields.vdd_w_curr_min << 5) |
             (ptcsd->Fields.vdd_w_curr_max << 2) |
             ((ptcsd->Fields.c_size_mult & 6) >>1); 

   buf[5]  = ((ptcsd->Fields.c_size_mult & 1) << 7);

   //bits 46-32 are different for SD and MMC cards.
   if(fSDcard)
    {
      buf[5]  |= ((ptcsd->Fields.erase.v22.erase_grp_size & 1) << 6) |
                ((ptcsd->Fields.erase.v22.sector_size & 0x7e)>>1);

      buf[4]  = ((ptcsd->Fields.erase.v22.sector_size & 1) << 7) |
                (ptcsd->Fields.wp_grp_size & 0x7F);
    }    
    else //MMC card
    {
      switch ( ptcsd->Fields.csd_structure ) {
        case CSD_STRUCT_VER_1_0:
        case CSD_STRUCT_VER_1_1:
             buf[5] |= ((ptcsd->Fields.erase.v22.sector_size) << 2) |
                       ((ptcsd->Fields.erase.v22.erase_grp_size & 0x18)>>3);

             buf[4] = (ptcsd->Fields.erase.v22.erase_grp_size & 7) <<5;
          break;
        case CSD_STRUCT_VER_1_2:
        default:
             buf[5] |= ((ptcsd->Fields.erase.v31.erase_grp_size) << 2) |
                       ((ptcsd->Fields.erase.v31.erase_grp_mult & 0x18)>>3);

             buf[4] = (ptcsd->Fields.erase.v31.erase_grp_mult & 7) <<5;
          break;
      }
          buf[4] |= (ptcsd->Fields.wp_grp_size & 0x1f);
    }//end of if sd card.

    buf[3] = (ptcsd->Fields.wp_grp_enable << 7) |
             ((ptcsd->Fields.default_ecc & 0x3) << 5)|
             (ptcsd->Fields.r2w_factor << 2)|
             ((ptcsd->Fields.write_bl_len & 0xc)>>2);

    buf[2] = ((ptcsd->Fields.write_bl_len & 0x3)<<6) |
             (ptcsd->Fields.write_bl_partial<<5);


    buf[1] = (ptcsd->Fields.file_format_grp << 7) |
             (ptcsd->Fields.copy << 6) |
             (ptcsd->Fields.perm_write_protect << 5) |
             (ptcsd->Fields.tmp_write_protect << 4) |
             (ptcsd->Fields.file_format << 2) |
             (ptcsd->Fields.ecc); 
    
	return ;
}

//==============================================================

//Data buffer should be byte aligned and should be of size
//= Data width + 8. eg for CSD write, it should be 120+8=128 bits.

int calcCrc7(unsigned char *DataBuff, int numBits) 
{ 
  int  popedOne; 
  int crc7 = 00; 
  int polynomial = 0x89;
  int i,j,k; 
  unsigned char Bytedata=0;

  //Update the crc register for each input bit 
  //Data buffer should hold MSB first.
  k=0;
  Bytedata = DataBuff[k];

  for(i=0,j=0;i<(numBits + 7); i++) 
  { 
    //Check if msb (bit popped after shifting) is 1 
    if (crc7 & 0x40) 
      popedOne = 1;  
    else popedOne = 0;

    //Take the next bit in the lsb position 
    crc7 = (crc7 << 1) & 0x7F; 

    //crc7[0] = DataBuff[i]; 
    if((Bytedata & (0x80>>j))>>(7-j))
      crc7 |= 1;
    else
      crc7 &= 0xFE; 

    if(j==7)
     {
      j=0;
      k++; 
      if(k <((numBits+8)/8) )
       Bytedata = DataBuff[k];
     }
    else j++;


    //If a 1 was popped, XOR with the polynomial. 
    if (popedOne == 1) crc7 = (crc7 ^ polynomial) & 0x7F; 
  } 

  //Return the calculated CRC 
  return crc7; 
} 


#endif

/**************************************************************************/
