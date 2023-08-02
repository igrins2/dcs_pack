MACIE_MODE
0210 0031  release reset
ASIC_MODE
#0cf0 800f  Set mSPI div 16 and useReg
0cf0 8001  Set mSPI div 2 and useReg 
WAIT 10
#0cf1 0000  Set mSPI startCNT, startEdge, txTimeShift = 0
0cf1 0005   Set mSPI startCNT = 0, startEdge = 1, txTimeShift = 1
WAIT 10
#0cec c000  Set sysClk div 0
0cec c180  Set sysClk div 6 (REGSYSCLKDIV = 3)
#0cec c200  Set sysClk div 8 (REGSYSCLKDIV = 4)
#0cec c300  Set sysClk div 12 (REGSYSCLKDIV = 6)
WAIT 10
MACIE_MODE
0101 000c  Set mSPI div 12
#0103 0010  Set mSPI div 16
#0103 0018  Set mSPI div 24
#0103 0080  set mSPI div 128
