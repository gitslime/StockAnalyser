#include "common/comm.h"
#include "common/file.h"

ULONG GetFileType(IN CHAR* szType)
{
    ULONG ulFileType;
    
    if (0 == _stricmp(szType, "ths_day")) {
        ulFileType = FILE_TYPE_THS_DAY;
    }
    else if (0 == _stricmp(szType, "ths_min5")) {
        ulFileType = FILE_TYPE_THS_MIN5;
    }
    else if (0 == _stricmp(szType, "custom")) {
        ulFileType = FILE_TYPE_CUSTOM;
    }
    else if (0 == _stricmp(szType, "weight")) {
        ulFileType = FILE_TYPE_QL_WGT;
    }
    else {
        printf("invaild file type\n");
        ulFileType = INVAILD_ULONG;
        exit(2);
    }
    return ulFileType;
}

VOID PrintSingleDay(IN FILE_WHOLE_DATA_S *pstEntry)
{
    ULONG i, ulMin30Cnt;
    ULONG aulMin[8] = {1000,1030,1100,1130,1330,1400,1430,1500};

    for (ulMin30Cnt=0;ulMin30Cnt<8;ulMin30Cnt++) {
        if (FILE_VAILD_PRICE != pstEntry->astMin30Price[ulMin30Cnt].ulFlag) break;
    }

    printf("\n=============================================================================");

    printf("\nDATE: %lu", pstEntry->ulDate);
    for (i=0;i<ulMin30Cnt;i++) {
        printf("\t%lu", aulMin[i]);
    }
    printf("\n=============================================================================");
    
    printf("\nBEGIN:\t%lu", pstEntry->stDailyPrice.ulBegin);
    for (i=0;i<ulMin30Cnt;i++) {
        printf("\t%lu", pstEntry->astMin30Price[i].ulBegin);
    }
    printf("\nHIGH:\t%lu", pstEntry->stDailyPrice.ulHigh);
    for (i=0;i<ulMin30Cnt;i++) {
        printf("\t%lu", pstEntry->astMin30Price[i].ulHigh);
    }
    printf("\nLOW:\t%lu", pstEntry->stDailyPrice.ulLow);
    for (i=0;i<ulMin30Cnt;i++) {
        printf("\t%lu", pstEntry->astMin30Price[i].ulLow);
    }
    printf("\nEND:\t%lu", pstEntry->stDailyPrice.ulEnd);
    for (i=0;i<ulMin30Cnt;i++) {
        printf("\t%lu", pstEntry->astMin30Price[i].ulEnd);
    }
    printf("\nVOL:\t%lu", pstEntry->stDailyPrice.ulVol);
    for (i=0;i<ulMin30Cnt;i++) {
        printf("\t%lu", pstEntry->astMin30Price[i].ulVol);
    }
    printf("\n");

    if (FILE_VAILD_FACTOR == pstEntry->stFactor.ulFlag) {
        printf("Multi=%f, Adder=%f\n", pstEntry->stFactor.fMulti, pstEntry->stFactor.fAdder);
    }

    return;
}

VOID ShowThsDaily(IN ULONG ulDate, IN ULONG ulEntryCnt, IN FILE_THS_DAILY_ENTRY_S *pstEntry)
{
    ULONG i;
    FILE_WHOLE_DATA_S stWholeData;

    //find entry
    for (i=0;i<ulEntryCnt;i++) {
        if (ulDate == pstEntry->ulDate) break;
        pstEntry++;
    }

    if (i==ulEntryCnt) {
        printf("date %ld not found\n", ulDate);
        return;
    }

    memset(&stWholeData, 0 ,sizeof(stWholeData));
    FILE_ThsDayToCustom(pstEntry, &stWholeData);
    PrintSingleDay(&stWholeData);

    return;
}

VOID ShowThsMin30(IN ULONG ulDate, IN ULONG ulEntryCnt, IN FILE_THS_MIN5_ENTRY_S *pstEntry)
{
    ULONG i;
    FILE_WHOLE_DATA_S stWholeData;
    ULONG ulShowMin;
    ULONG ulStartDate = FILE_ThsMin2Date(pstEntry->ulMin);

    if (ulStartDate > ulDate) {
        printf("Entry starts at %ld.\n", ulStartDate);
        return;
    }
    ulShowMin = FILE_Date2ThsMin(ulDate);

    for (i=0;i<ulEntryCnt;i++,pstEntry++) {
        if (pstEntry->ulMin > ulShowMin) break;
    }
    
    if (i==ulEntryCnt) {
        printf("date %ld not found\n", ulDate);
        return;
    }

    memset(&stWholeData, 0 ,sizeof(stWholeData));
    stWholeData.ulDate = ulDate;
    FILE_ThsMin5ToMin30(pstEntry, &stWholeData);
    PrintSingleDay(&stWholeData);
    return;
}

VOID ShowQlWeight(IN ULONG ulDate, IN ULONG ulEntryCnt, IN FILE_QL_WEIGHT_ENTRY_S *pstEntry)
{
    return;
}

VOID ShowCustom(IN ULONG ulDate, IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstEntry)
{
    ULONG ulIndex;
    FILE_WHOLE_DATA_S *pstCurrent;
    
    ulIndex = GetIndexByDate(ulDate, INDEX_EXACT, ulEntryCnt, pstEntry);
    if (INVAILD_ULONG == ulIndex) {
        printf("date %ld not found\n", ulDate);
        return;
    }
    pstCurrent = pstEntry+ulIndex;

    PrintSingleDay(pstCurrent);

    return;
}

VOID ShowFileContent(IN ULONG ulStockCode, IN ULONG ulDate, IN CHAR *szDir, IN ULONG ulFileType)
{
    ULONG ulEntryCnt;
    VOID *pData = NULL;

    ulEntryCnt = FILE_GetFileData(ulStockCode, szDir, ulFileType, &pData);
    if (0 == ulEntryCnt) {
        printf("can't get data\n");
        return;
    }
    
    switch(ulFileType) {
        case FILE_TYPE_THS_DAY:
            ShowThsDaily(ulDate, ulEntryCnt, pData);
            break;
        case FILE_TYPE_THS_MIN5:
            ShowThsMin30(ulDate, ulEntryCnt, pData);
            break;
        case FILE_TYPE_QL_WGT:
            ShowQlWeight(ulDate, ulEntryCnt, pData);
            break;
        case FILE_TYPE_CUSTOM:
            ShowCustom(ulDate, ulEntryCnt, pData);
            break;
        default:
            assert(0);
    }

    free(pData);
    return;
}

int main(int argc,char *argv[])
{
    ULONG ulStockCode;
    ULONG ulShowDate;
    ULONG ulFileType;
    
    //check parameter
    if ((argc < 5) || (argc > 6)) {
        printf("USAGE: %s path type code date [debug]", argv[0]);
        exit(1);
    }
    if (0 == _stricmp(argv[argc-1], "debug"))
        g_bIsDebugMode = BOOL_TRUE;
    
    ulStockCode = (ULONG)atol(argv[3]);
    ulShowDate  = (ULONG)atol(argv[4]);
    ulFileType  = GetFileType(argv[2]);
    DebugOutString("code=%06lu, date=%lu, type=0x%08lx\n", ulStockCode, ulShowDate, ulFileType);

    ShowFileContent(ulStockCode, ulShowDate, argv[1], ulFileType);

    return 0;
}

