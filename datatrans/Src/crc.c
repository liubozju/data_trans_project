//-------------include---------------//
#include "crc.h"

/**
  * @brief  �����CRC_TYPE���͵�CRC���
  * @param  CRCIn��CRC��ʼֵ
  * @param  buf����ҪCRC������
  * @param  len����ҪCRC�����鳤��
  * @param  polyHex������ʽ��  0x8005
  * @retval CRC���ս��CRC_TYPE
  */
CRC_TYPE CalCrc(uint16_t CRCIn, c8 * buf, u16 len,u16 polyHex)
{
    u16 byte;
    u8 k;
    u32 ACC,TOPBIT;
    u16 remainder = CRCIn;//1����ʼֵ
    TOPBIT = 0x8000;  //����ȡ���λ
    for (byte = 0; byte < len; ++byte)
    {
        ACC = buf[byte];
        remainder ^= (ACC <<8); // 2�������ݵ�һ���ֽں�CRCIn��8λ���
        for (k = 8; k > 0; --k)
        {
            if (remainder & TOPBIT)//3��ȡ���λ��Ϊ1�������ʽHex�����
            {
                remainder = (remainder << 1) ^polyHex;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }
    }
    remainder=remainder^0x0000;//����ԭ���Ǹ���
	
    return remainder;
}

