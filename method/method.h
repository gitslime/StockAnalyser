#ifndef __METHOD_H__
#define __METHOD_H__

VOID RISE_Statistics(IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstBeginData, IN FILE_WHOLE_DATA_S *pstFirstData);
BOOL_T RISE_Choose(IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo);


#endif 
