#include <Constants.au3>

HotKeySet("{ESC}", "quit")
Func quit ()
   Exit
EndFunc

Global $MainTitle = "机构交易专业版"

Func XiadanLogin ()
   Run("D:\green\xiadan\xiadan.exe")
   
   Local $LoginTitle = "用户登录"  
   
   ; Log in
   WinActivate($LoginTitle)
   WinWaitActive($LoginTitle)
   Send("hexin{TAB}888888{ENTER}")
   Sleep(2000)
   
EndFunc

Func XiadanInitAccount ()
   Local $Accaunt  = "11076683"
   Local $DealPass = "292623"
   Local $CommPass = "Vfr45tgB"
   
   ; set enviroment
   AutoItSetOption ("MouseCoordMode", 0)    ; relative coords to the active window

   ; enter account
   MouseClick("primary", 482, 174)
   Send($Accaunt  &"{TAB}")
   Send($DealPass &"{TAB}")
   Send($CommPass &"{TAB}")
   Send("{ENTER}")
   Sleep(1000)
   Send("{ENTER}")
   
EndFunc

Func XiadanGotoPreDeal ()
   ; set enviroment
   AutoItSetOption ("MouseCoordMode", 0)    ; relative coords to the active window
   
   if Not WinExists($MainTitle) Then
	  MsgBox(0, "Error", $MainTitle & "未启动" )
   EndIf
   
   ; go to pre-deal
   WinActivate($MainTitle)
   MouseClick("primary", 250, 50)
   Sleep(100)
   MouseClick("primary", 250, 205)
   Sleep(100)
   MouseClick("primary", 440, 140)
EndFunc

Func XiadanInitPreDeal ()
   XiadanGotoPreDeal()
   
   ; click time&price
   Send("{TAB 6}")
   Send("{SPACE}")
   Send("{TAB 3}")
   Send("{SPACE}")
   Send("{TAB 6}")
EndFunc

Func XiadanSetPreDeal($Code, $IsSell, $Hour, $Min, $IsHigher, $Price)
   ;MsgBox(0, "param", $Code&$IsSell&$Hour&$Min&$IsHigher&$Price)
   Local $SetPrice  = 100
   Local $Cnt = Floor($SetPrice/$Price) * 100
   
   ;If $Cnt*$Price < 9200 Then Return 0
   If $Cnt*$Price < 9200 Then $Cnt+=100
   
   Send($Code)
   Sleep(200)
   Send("{TAB 3}")
   Send($Cnt)
   Send("{TAB 2}")
   If $IsSell == 0 Then
	  Send("1")		; buy
   Else
	  Send("2")		; sell
   EndIf
   Sleep(100)
   Send("{TAB 2}"&$Hour&"{TAB}"&$Min)
   Send("{TAB 2}")
   If $IsHigher == 0 Then
	  Send("2")		; lower
   Else
	  Send("1")		; higher
   EndIf
   Sleep(100)
   Send("{TAB}"&$Price)
   Send("!S")		; ALT + S
   
   WinWaitActive($MainTitle)
   Sleep(200)
   Send("{ENTER}")
   Sleep(100)

   Local $Text = WinGetText($MainTitle)
   if StringInStr($Text, "自动交易埋单成功，") Then
	  Send("{ENTER}")
   EndIf
   
   Return 1
   
EndFunc

;; function retrun an 2-ray array
;; array[0][0] = count of hold stocks
;; array[1][0] = stock code
;; array[1][1] = stock amount
Func XiadanGetHold ()
  
   ; set enviroment
   AutoItSetOption ("MouseCoordMode", 0)    ; relative coords to the active window
   
   if Not WinExists($MainTitle) Then
	  MsgBox(0, "Error", $MainTitle & "未启动" )
	  Return
   EndIf
   
   WinActivate($MainTitle)
   Send("{F9}")
   MouseClick("primary", 800, 400)
   Send("^c")					;; copy hold list to clipboard
   Local $HoldList = ClipGet()
   Local $Line = StringSplit($HoldList, @CRLF , 1)		; get each line
   ;MsgBox(0, "Total", $HoldList)
   
   Local $i = 2		; skip first line
   Local $HoldList[60][2]
   While ($i <= $Line[0])
	  Local $Stock = StringSplit($Line[$i], @TAB)
	  Local $HoldId = $Stock[2]
	  ;MsgBox(0, "code", $Stock[4]&$Stock[7])
	  $HoldList[$HoldId][0]=$Stock[4]
	  $HoldList[$HoldId][1]=$Stock[7]
	  
	  $i+=1
   WEnd
   $HoldList[0][0]=$HoldId
   
   Return $HoldList
EndFunc

Func XiadanClearHold ()
   ; set enviroment
   AutoItSetOption ("MouseCoordMode", 0)    ; relative coords to the active window
   
   if Not WinExists($MainTitle) Then
	  MsgBox(0, "Error", $MainTitle & "未启动" )
   EndIf
   
   Local $HoldList[100][100]
   $HoldList = XiadanGetHold()
   Local $HoldCnt=$HoldList[0][0]
   Send("{F10}")
   Sleep(50)
   
   For $i=1 To $HoldCnt
	  MouseClick("primary", 400, 138)
	  ;MsgBox(0, "debug", $i&" "&$HoldCnt)
	  For $j=2 To $i 
		 Send("{DOWN}")
	  Next
	  Send("!A")
	  Sleep(50)
	  Send("{TAB 4}")
	  Send("{SPACE}")
	  Send("{TAB}")
	  Send("{SPACE}")
	  Send("{TAB 3} 5")
	  Send("{TAB 2} 3")
	  Send("!Y")
	  Sleep(500)
   Next
   
EndFunc

; returns an array, array[0]=total count of trades, array[i]=one line of trade info
Func XiadanGetTradeInfobyFile ()
   Local $FILE_Choose = "C:\Users\slime\0422.txt"
   Local $fp = FileOpen($FILE_Choose, 0)
   
   ; Check if file opened for reading OK
   If $fp = -1 Then
	  MsgBox(0, "Error", "Unable to open file " & $FILE_Choose)
	  Exit
   EndIf   
   
   ; read line by line
   Local $i=1
   Local $Trade[3000]
   While 1
	  $Trade[$i] = FileReadLine($fp)
	  If @error = -1 Then ExitLoop
		 
	  $i+=1
   WEnd
   $Trade[0] = $i-1
   
   FileClose($fp)
   
   Return $Trade
EndFunc

Func XiadanGetTradeInfobyProgram ()
   Local $TotalCnt=0
   Local $Trade[3000]
   
   ;Local $Date=@YEAR&@MON&@MDAY
   Local $Date=20130627
   ;MsgBox(0, "Date", $Date)
   
   Local $foo = Run("F:\StockAnalyser\build\StockAnalyser\Debug\Choose.exe rise F:\StockAnalyser\database "&$Date&" all", "", @SW_HIDE, $STDOUT_CHILD)
   Local $Output
   While 1
	  $Output = StdoutRead($foo)
	  If @error Then ExitLoop
	  
	  ;MsgBox(0, "READ", $Output)
	  Local $line = StringSplit($Output, @CRLF, 1)
	  $line[0]-=1				; Skip last blank line
	  For $i = 1 To $line[0]
		 $TotalCnt+=1
		 $Trade[$TotalCnt]=$line[$i]
		 ;MsgBox(0, "Info", "Get "& $Trade[$TotalCnt])
	  Next
   WEnd
   $Trade[0]=$TotalCnt
   ;MsgBox(0, "Info", "Total Count="&$TotalCnt)
   
   Return $Trade
EndFunc

Func XiadanDailyTrade()
   Local $TradeCnt = 60
   Local $TradeInfo = XiadanGetTradeInfobyProgram()
   Local $TotalTrade = $TradeInfo[0]
   Local $SkipStep = Round($TotalTrade/$TradeCnt)
   If $SkipStep = 0 Then $SkipStep=1
   
   Local $i=0
   Local $ActualTrade=0
   While $i<$TotalTrade
	  $i+=1
	  
	  Local $param = StringSplit($TradeInfo[$i], ",")
	  If $param[0] <> 6 Then
		 ;MsgBox(0, "Error", $TradeInfo[$i])
		 ContinueLoop
	  EndIf
	  	  
	  if Mod($i, $SkipStep) Then ContinueLoop
		 
	  $ActualTrade+=XiadanSetPreDeal($param[1],$param[2],$param[3],$param[4],$param[5],$param[6])
   WEnd
   
   ;MsgBox(0, "debug", $ActualTrade)
   XiadanAutoDraw($ActualTrade)
EndFunc

Func XiadanAutoDraw ($Cnt)
   If Not WinExists($MainTitle) Then
	  MsgBox(0, "Error", "Windows not exist")
	  Exit
   EndIf
   WinActivate($MainTitle)
   
   ; set enviroment
   AutoItSetOption ("MouseCoordMode", 0)    ; relative coords to the active window
   
   Send("{F8}")
   Sleep(100)
   MouseClick("primary", 500, 160)
   Sleep(100)
   Send("{UP 100}")
   
   Local $i=0
   While $i < $Cnt 
	  Send("G")
	  WinWaitActive("修改自动交易")
	  Send("{TAB 5}9")
	  Send("!Y")
	  Sleep(200)
	  Send("{DOWN}")
	  Sleep(50)
	  $i+=1
   WEnd
   
EndFunc

FileDelete("D:\green\xiadan\hexin\data.jx")
XiadanLogin()
XiadanInitAccount()
XiadanInitPreDeal()
XiadanDailyTrade()
;XiadanClearHold()
