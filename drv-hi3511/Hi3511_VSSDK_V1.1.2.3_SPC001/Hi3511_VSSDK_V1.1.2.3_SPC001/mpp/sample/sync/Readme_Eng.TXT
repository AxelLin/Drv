-------------------------------------------------------------
Description of the Sample of Playing Control
-------------------------------------------------------------

1 The video output (VO) channels decode streams based on presentation time stamps (PTSs).
1) If a stream contains PTSs, the PTSs are converted into intervals according to the frame rate set by users, and the frames containing the PTSs are output in a specific time.
2) If a stream does not contain PTSs, PTSs are set in VO channels. This method is a compensation for output control. In this output mode, however, the streams of non-full frames 
    cannot be controlled accurately.
3) When multiple channels are synchronized, you should set a PTS of a channel to the time basis. As a result, the channels in the synchronization group output streams synchronously
   at the configured frame rate.
4) To fast forward streams, you must set a frame rate by calling call HI_MPI_VO_SetFrameRate and then send only the I frames in the streams to the decoder.

2 To play streams in the following four modes, you need to only set frame rates by calling the following application programming interfaces (APIs). Different frame rates map to 
   different playing speeds. 

   HI_S32 HI_MPI_VO_SetFrameRate (VO_CHN VoChn, HI_U32 u32VoFramerate)

1) Normal
	By default, the VO channels output streams at full frame rate in each standard.
2) Slow
	The frame rate ranges from 1fps to full frame rate.
3) Fast forward
	The frame rate ranges from full frame rate to 64x full frame rate.
	Note: Due to the restriction of the decoder, the highest decoding speed is 4x for D1 format, 16x for CIF format, and 64x for QCIF format.
4) Back forward
	The frame rate ranges from ¨C1x full frame rate to ¨C64x full frame rate.
	Note: In this mode, you must send frames to the decoder in reverse order. When the performance of the decoder is limited, you can implement back forward playing by 
              sending I frames only.

3 The playing speed is not changed in the following playing modes. You can use the three modes by calling the following APIs:

1) Pause
	HI_S32 HI_MPI_VO_ChnPause (VO_CHN VoChn)
2) Resume
	HI_S32 HI_MPI_VO_ChnResume (VO_CHN VoChn)
3) Step
	HI_S32 HI_MPI_VO_ChnStep (VO_CHN VoChn)

4 To play streams synchronously, do as follows:
Step 1 Create a synchronization group by calling HI_S32 HI_MPI_VO_CreateSyncGroup (VO_GRP VoGroup, VO_SYNC_GROUP_ATTR_S *pstSycGrpAttr).
Step 2 Register VO channels with the synchronization group by calling HI_S32 HI_MPI_VO_RegisterSyncGroup (VO_CHN VoChn, VO_GRP VoGroup).
Step 3 Enable the synchronization group by calling HI_S32 HI_MPI_VO_SyncGroupStart (VO_GRP VoGroup).
Step 4 Set the frame rate of the synchronization group by calling HI_S32 HI_MPI_VO_SetSyncGroupFrameRate (VO_GRP VoGroup, HI_U32 u32VoGrpFramerate).
----End

5 You can set the fast forward speed.
   To fast forward streams at 4x or 8x speed, do as follows:
Step 1 Read the I frames from streams. Each I frame must have a PTS.
Step 2 Set a required playing speed by calling HI_MPI_VO_SetFrameRate.
----End
	
6 You can use the back forward mode as required.
   To back forward streams, do as follows:
Step 1 Read the current I frame from streams and output it.
Step 2 Set a required playing speed by calling HI_MPI_VO_SetFrameRate. 
          This speed is a negative value.
Step 3 If you back forward streams in full I frame mode, locate the previous frame in streams to check whether it is an I frame. If yes, send it to the decoder; otherwise, discard it.
----End

Precautions:
   The accuracy of the fast forward mode is affected by the I frame interval, namely, the value of group of pictures (GOP).
   
7 Operation instructions
1) Double-click sample_sync, and then click menu options to perform corresponding playing control. If an exception occurs and the program exits, run the 
    sample_sync reset command to reset the program.
2) The source codes for generating the required video streams and audio/video encoding (AVENC) streams are stored in the producer directory.
3) Copy the generated video streams and AVENC streams to the sync directory.

8 Directory structure
	¡ö sample
	|--Makefile
	|--sample_sync.c
	|-- producer
	  |--av_producer.c
	  |--Makefile
	  |--Makefile.param
	  |--v_producer.c

9 The following is the structure of the reference frame of an AVENC stream:

	/***************************************************************
	    @ structure of av frame package @
	
	          |<-AN AV FRAME--------------------------->|
	          |  |<-VIDEO---->|<--AUDIO->|...|<-AUDIO-->|
	          |                                         |
	          |  |v-header    |a-header  |...|a-header  |
	    ------|--|-|----------|-|--------|...|-|--------|--------
	          |  |                                      |
	           ^
	           |
	           av-header
	***************************************************************/

10 For any question, please feel free to send email to xujilin@huawei.com.
