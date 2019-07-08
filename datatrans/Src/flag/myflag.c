#include  "myflag.h"
#include  "string.h"
#include  "upgrade.h"
#include "can.h"

extern gFlag_Data gFlag;

void platformFlagInit(void )
{
	memset(&gFlag,0,sizeof(gFlag));
	STMFLASH_Read(GLOBAL_FLAG,(uint8_t *)&gFlag,sizeof(gFlag));
	LOG(LOG_INFO,"gFlag.ModeFlag:%x gFlag.DiffUpgradeStartFlag:%x gFlag.DiffUpgradeEndFlag:%x\r\n",gFlag.ModeFlag,gFlag.DiffUpgradeStartFlag,gFlag.DiffUpgradeEndFlag);
	if(gFlag.ModeFlag != SD_MODE_FLAG && gFlag.ModeFlag != NET_MODE_FLAG)
	{
		gFlag.ModeFlag = NET_MODE_FLAG;
		STMFLASH_Write_WithBuf(GLOBAL_FLAG,(uint8_t *)&gFlag,sizeof(gFlag));
	}
}

void gWriteDiffStartFlag(uint8_t sFlag)
{
	/*离线开始存储标记*/
	gFlag.DiffUpgradeStartFlag = DIFF_START_FLAG;
	STMFLASH_Write_WithBuf(GLOBAL_FLAG,(uint8_t *)&gFlag,sizeof(gFlag));
}

void gWriteDiffEndFlag(uint8_t sFlag)
{
	/*离线结束存储标记--与CAN接口发送ID*/
	gFlag.DiffCannum = can_id.Can_num;
	gFlag.DiffCanSendid = can_id.SendID;
	gFlag.DiffCanRevid = can_id.RecID;
	gFlag.DiffUpgradeEndFlag = DIFF_END_FLAG;
	STMFLASH_Write_WithBuf(GLOBAL_FLAG,(uint8_t *)&gFlag,sizeof(gFlag));
}

void gClearDiffFlag(void)
{
	gFlag.DiffUpgradeEndFlag = DIFF_CLEAR_FLAG;
	gFlag.DiffUpgradeStartFlag = DIFF_CLEAR_FLAG;
	STMFLASH_Write_WithBuf(GLOBAL_FLAG,(uint8_t *)&gFlag,sizeof(gFlag));
}









