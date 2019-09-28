# Lumberjack
This is an implementation of the Lumberjack game (on Telegram) that is played on an FPGA board (DE1-SoC). 
The player uses the FPGA board's keys as input and the game's graphics are displayed on a VGA monitor.
The program is coded in C, and is essentially a hardware description of the game. The program manipulates
various components of the DE1-SoC board such as its; keys, HEX displays, private/interval timers, and VGA output. 
