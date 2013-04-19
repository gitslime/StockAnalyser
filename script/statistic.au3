#include <stock_list.au3>

Global $Static = "F:\StockAnalyser\build\StockAnalyser\Debug\Statistics.exe "
Global $Method  = " rise "
Global $DataPath = " F:\StockAnalyser\database "

Local $StockList = GetStockList()
Local $i = 1

While $i <= $StockList[0]
   Local $StockCode = $StockList[$i]
   Local $OutputFile = " > F:\StockAnalyser\statistics\"&$StockCode&".csv"
   Local $FullCmd = @ComSpec&" /c "&$Static&$Method&$DataPath&$StockCode
   
   ;MsgBox(0, "CMD", $FullCmd&$OutputFile)
   Run($FullCmd&$OutputFile,"",@SW_HIDE) 

   Sleep(50)
   $i+=1
   
   ; control process count
   Local $Process = ProcessList ("Static.exe")
   if $Process[0][0] > 15 Then
	  ;MsgBox(0,"Process List", $Process[0][0]);
	  Sleep(1000)
   EndIf
   
   if WinExists("Microsoft Visual C++ Runtime Library") Or WinExists("OrgData.exe") Then
	  MsgBox(0, "Error", $StockCode)
   EndIf

WEnd