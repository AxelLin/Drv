һ��sample����˵��
audio         ��Ƶ����������������
avenc         ����Ƶ������
codec         264 ͬ��ͬ��
cursor        �����ʾ
deflicker     ����˸Ч����ʾ
md            �˶����
misc          ��ʾVideoBuffer������ã���������ɢ����
pciv          pci ����
split_screen  GUI ����ָ���ʾ
sync          ��·��Ƶͬ����ʾ
vdec          ����264�ļ���ʾ
venc          264���롢MJPEG���롢JPEGץ��
vio           ��Ƶ���������ʾ
vpp           OSD ��ʾ

�����޶���¼
�޶�����: 2009-05-14
�޶�����:
    1, �޸�264�������Ϊ�Ƽ�ֵ
        bCBR = HI_TRUE;
        picLevel = 0;  
        gop = 100;
        bitate = 1024kbps(D1) / 512kbps(CIF);
    
    2, �޸�264�������Ϊ�Ƽ�ֵ
        RefFrameNum = 2 (�������);
        
    3, vio ����Ԥ�������л���ʾ��8D1ʵʱԤ������֡�ʱ�����ʾ��
    
    4, vdec ����EOS���ͣ���֤�ļ����Խ��ꡣ
    
    5, ɾ����������sample���롣
    
    
