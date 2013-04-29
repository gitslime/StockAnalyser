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
   
   If $Cnt*$Price < 9200 Then Return
   
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
   
EndFunc

;; function retrun an 2-ray array
;; array[0][0] = count of hold stocks
;; array[1][0] = stock code
;; array[1][1] = stock amount
Func XiadanGetHold ()
   
   ; set enviroment
   AutoItSetOption ("MouseCoordMode", 0)    ; relative coords to the active window
   
   if WinExists($MainTitle) Then
	  MsgBox(0, "Error", $MainTitle & "未启动" )
   EndIf
   
   WinActivate($MainTitle)
   Send("{F9}")
   MouseClick("primary", 800, 400)
   Send("^c")					;; copy hold list to clipboard
   Local $HoldList = ClipGet()
   Local $Line = StringSplit($HoldList, @CRLF , 1)		; get each line
   MsgBox(0, "Total", $HoldList)
   
   Local $i = 2		; skip first line
   While ($i <= $Line[0])
	  Local $Stock = StringSplit($Line[]
	  
	  $i+=1
   WEnd
   
   
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

Func XiadanDailyTrade()
   Local $TradeCnt = 60
   Local $TradeInfo = XiadanGetTradeInfobyFile()
   Local $TotalTrade = $TradeInfo[0]
   Local $SkipStep = Round($TotalTrade/$TradeCnt)
   
   Local $i=0
   While $i<$TotalTrade
	  $i+=1
	  
	  Local $param = StringSplit($TradeInfo[$i], ",")
	  If $param[0] <> 6 Then
		 MsgBox(0, "Error", $TradeInfo[$i])
		 ContinueLoop
	  EndIf
	  	  
	  if Mod($i, $SkipStep) Then ContinueLoop
		 
	  XiadanSetPreDeal($param[1],$param[2],$param[3],$param[4],$param[5],$param[6])
   WEnd

EndFunc

FileDelete("D:\green\xiadan\hexin\data.jx")
XiadanLogin()
XiadanInitAccount()
XiadanInitPreDeal()
XiadanDailyTrade()
