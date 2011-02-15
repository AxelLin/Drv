－－－－－－－－－－－－－
播放控制sample说明
－－－－－－－－－

1、VO按照时间戳进行解码回放
（1）如果码流中带有时间戳，根据用户设置帧率，转换成相应的时间间隔，将含有时间戳信息的帧在应该的时刻输出显示。
（2）如果码流没有时间戳，VO会在内部进行加上时间戳，作为输出控制的一种补偿。但是，这种输出对于非满帧码流不能准确控制。
（3）多通道同步时，选择某一通道作为时间基准起点，然后所有同步组中的通道按照同步组设定帧率同步输出。
（4）对于多倍数播放，可以通过SDK的设置帧率接口实现，采用I帧方式进行播放输出的控制方式需要用户参实现全I帧的送入。

2、以下四种播放控制都可以直接调用下面的接口实现，只要设置帧率为相应的速度即可。 

   HI_S32 HI_MPI_VO_SetFrameRate (VO_CHN VoChn,HI_U32 u32VoFramerate)

（1）正常（normal）
	默认情况下，VO按照各种制式下的满帧率输出。
（2）慢放（slow）
	设置帧率从 【1fps～满帧】
（3）快放（fast forward）
	设置帧率从【满帧率～64x满帧率】
	注意：受解码器性能限制，D1最大的全解性能为4x，CIF为16x，QCIF为64x。
（4）倒放（back forward)
	设置帧率从【－1x满帧率～－64x满帧率】
	注意：送入帧的倒序由用户保证，在解码性能受限的情况下，通常使用送I帧的方式实现倒退播放。

3、以下三种方式对播放速度不做改变，其设置可以直接调用VO接口。

（1）暂停（pause）
	HI_S32 HI_MPI_VO_ChnPause (VO_CHN VoChn)
（2）恢复（resume）
	HI_S32 HI_MPI_VO_ChnResume (VO_CHN VoChn)
（3）步进（step）
	HI_S32 HI_MPI_VO_ChnStep (VO_CHN VoChn)

4、同步播放可以通过创建同步组、注册同步通道、启动同步等一系列接口调用实现，典型的API如下。
（1）创建同步组 
	HI_S32 HI_MPI_VO_CreateSyncGroup(VO_GRP VoGroup, VO_SYNC_GROUP_ATTR_S *pstSycGrpAttr)
（2）注册VO通道到同步组
	HI_S32 HI_MPI_VO_RegisterSyncGroup(VO_CHN VoChn, VO_GRP VoGroup)
（3）启动同步组
	HI_S32 HI_MPI_VO_SyncGroupStart(VO_GRP VoGroup)
（4）设置同步组帧率
	HI_S32 HI_MPI_VO_SetSyncGroupFrameRate(VO_GRP VoGroup, HI_U32 u32VoGrpFramerate)

5、高倍速播放控制由用户实现。
   在进行4x，8x解码回放的时候，具体步骤如下。
（1）从码流文件中读取I帧，送入的I帧需要有时间戳。。
（2）采用HI_MPI_VO_SetFrameRate接口设置需要的播放速度。
	
6、倒退播放由用户实现。
   在进行倒退处理的时候，具体步骤如下。
（1）从码流文件中读取当前I帧并输出显示。
（2）采用HI_MPI_VO_SetFrameRate接口设置需要的播放速度，这时的帧率为负数。
（2）如果用户采用全I帧的方式倒退，则在码流文件中定位到上一个帧，判断是否I帧，是则送出显示，否则丢弃。

注意事项：
	I帧间隔，即GOP值会影响高倍速快进的精度。
   
7、操作说明。
（1）在sample_sync中，根据菜单项进行各种播放控制。如果出现异常退出，使用命令行reset参数重置程序状态。
（2）producer目录下包含了产生所需视频码流、复合码流的源代码。
（3）将产生后的视频码流、复合流拷贝到sync目录下。

8、目录结构。
	■ sample
	┣ Makefile
	┣ sample_sync.c
	┗■ producer
	  ┣ av_producer.c
	  ┣ Makefile
	  ┣ Makefile.param
	  ┗ v_producer.c

9、附复合流参考帧结构。

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

10、问题联系方式：
	Email: xujilin@huawei.com