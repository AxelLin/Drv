一、sample用例说明
audio         音频输入输出，编码解码
avenc         音视频复合流
codec         264 同编同解
cursor        鼠标演示
deflicker     抗闪烁效果演示
md            运动检测
misc          演示VideoBuffer如何配置，及其他零散功能
pciv          pci 传输
split_screen  GUI 画面分割演示
sync          多路视频同步显示
vdec          解码264文件演示
venc          264编码、MJPEG编码、JPEG抓拍
vio           视频输入输出演示
vpp           OSD 演示

二、修订记录
修订日期: 2009-05-14
修订描述:
    1, 修改264编码参数为推荐值
        bCBR = HI_TRUE;
        picLevel = 0;  
        gop = 100;
        bitate = 1024kbps(D1) / 512kbps(CIF);
    
    2, 修改264解码参数为推荐值
        RefFrameNum = 2 (快速输出);
        
    3, vio 增加预览画面切换演示，8D1实时预览、低帧率编码演示。
    
    4, vdec 增加EOS发送，保证文件可以解完。
    
    5, 删除部分冗余sample代码。
    
    
