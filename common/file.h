
#ifndef __FILE_H__
#define __FILE_H__

#define FILE_THS_DAY_ENTRY_TOTAL_LEN     (168)
#define FILE_THS_DAY_ENTRY_RSV_LEN       (FILE_THS_DAY_ENTRY_TOTAL_LEN - 28)
#define FILE_THS_MIN5_ENTRY_TOTAL_LEN    (136)
#define FILE_THS_MIN5_ENTRY_RSV_LEN      (FILE_THS_MIN5_ENTRY_TOTAL_LEN - 28)

#define FILE_THS_PRICE_CHK         (0xB0000000)
#define FILE_THS_VOL_CHK           (0xA0000000)
#define FILE_THS_GET_REAL_PRICE(ulFilePrice)     ((ulFilePrice)-FILE_THS_PRICE_CHK)

typedef struct tagFileDailyData
{
    ULONG  ulDate;
    ULONG  ulBeginWithCheck;
    ULONG  ulHighWithCheck;
    ULONG  ulLowWithCheck;
    ULONG  ulEndWithCheck;
    ULONG  ulRsv;
    ULONG  ulVolWithCheck;
    UCHAR  aucRsv[FILE_THS_DAY_ENTRY_RSV_LEN];
}FILE_THS_DAILY_ENTRY_S;

typedef struct tagFile5MinuteData
{
    ULONG  ulMin;
    ULONG  ulBeginWithCheck;
    ULONG  ulHighWithCheck;
    ULONG  ulLowWithCheck;
    ULONG  ulEndWithCheck;
    ULONG  ulRsv;
    ULONG  ulVolWithCheck;
    UCHAR  aucRsv[FILE_THS_MIN5_ENTRY_RSV_LEN];
}FILE_THS_MIN5_ENTRY_S;

typedef struct tagFileWeight
{
    ULONG ulQlDate;
    ULONG ulGift;           //10000 送股
    ULONG ulOffer;          //10000 配股
    ULONG ulOfferPrice;     //100   配股价
    ULONG ulProfit;         //1000  现金红利
    ULONG ulAdd;            //10000 转增
    ULONG ulTotalStock;     //总股本
    ULONG ulCirStock;       //流通股
    ULONG ulRsv;
}FILE_QL_WEIGHT_ENTRY_S;

ULONG FILE_GetVol(IN ULONG ulFileVol);
ULONG FILE_ThsMin2Date(IN ULONG ulMin);
ULONG FILE_Date2ThsMin(IN ULONG ulDate);
ULONG FILE_ThsMin5ToMin30(IN FILE_THS_MIN5_ENTRY_S *pstMin5, OUT FILE_WHOLE_DATA_S *pstWhole);
VOID FILE_ThsDayToCustom(IN FILE_THS_DAILY_ENTRY_S *pstDay, OUT FILE_WHOLE_DATA_S *pstWhole);
ULONG FILE_QlWeightToCustom(IN FILE_QL_WEIGHT_ENTRY_S *pstWeight, OUT FILE_WHOLE_DATA_S *pstWhole);
VOID FILE_SetFileData(IN ULONG ulStockCode,IN const CHAR *szDir,IN ULONG ulFileType,IN ULONG ulSetLen,IN VOID *pFileData);
ULONG FILE_GetFileData(IN ULONG ulStockCode, IN const CHAR *szDir, IN ULONG ulFileType, OUT VOID **ppFileData);
ULONG FILE_QlDate2Custom(IN ULONG ulWeightDate);


#endif

