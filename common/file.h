
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
    UINT32  ulDate;
    UINT32  ulBeginWithCheck;
    UINT32  ulHighWithCheck;
    UINT32  ulLowWithCheck;
    UINT32  ulEndWithCheck;
    UINT32  ulRsv;
    UINT32  ulVolWithCheck;
    UCHAR  aucRsv[FILE_THS_DAY_ENTRY_RSV_LEN];
}FILE_THS_DAILY_ENTRY_S;

typedef struct tagFile5MinuteData
{
    UINT32  ulMin;
    UINT32  ulBeginWithCheck;
    UINT32  ulHighWithCheck;
    UINT32  ulLowWithCheck;
    UINT32  ulEndWithCheck;
    UINT32  ulRsv;
    UINT32  ulVolWithCheck;
    UCHAR  aucRsv[FILE_THS_MIN5_ENTRY_RSV_LEN];
}FILE_THS_MIN5_ENTRY_S;

typedef struct tagFileWeight
{
    UINT32 ulQlDate;
    UINT32 ulGift;           //10000 送股
    UINT32 ulOffer;          //10000 配股
    UINT32 ulOfferPrice;     //100   配股价
    UINT32 ulProfit;         //1000  现金红利
    UINT32 ulAdd;            //10000 转增
    UINT32 ulTotalStock;     //总股本
    UINT32 ulCirStock;       //流通股
    UINT32 ulRsv;
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

