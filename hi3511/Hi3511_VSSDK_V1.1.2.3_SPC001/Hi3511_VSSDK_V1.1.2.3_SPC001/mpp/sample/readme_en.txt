
1,sample declaration
audio         AI,AO,AENC,ADEC
avenc         AVENC
codec         H.264 encode and decode 
cursor        cursor demo
deflicker     deflicker demo    
md            motion detective
misc          configure VideoBuffer,and miscellaneous  function
pciv          pci transmission
split_screen  GUI split screen
sync          display synchronization of multiple channel
vdec          Video decoder demo
venc          Video encoder and snap 
vio           Video input and output 	
vpp           OSD demo


2,revise record
revise date:2009-05-14
revise description:
    1)modify some recommendatory attribute of H.264 encoder
	bCBR = HI_TRUE;
        picLevel = 0;  
        gop = 100;
        bitate = 1024kbps(D1) / 512kbps(CIF);
    2)modify some recommendatory attribute of H.264 decoder
	RefFrameNum = 2 (fast export)
    3)sample_vio add two function
	a£¬switch display screen
	b, 8D1 real-time live, noreal-time encoder
    4)sample_vdec add sending EOS, ensure decoding over
    5)delete some redundant sample	
    
