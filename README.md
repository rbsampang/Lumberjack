# Lumberjack
This is an implementation of the Lumberjack game (on Telegram) that is played on an FPGA board (DE1-SoC). 
The player uses the FPGA board's keys as input and the game's graphics are displayed on a VGA monitor.
The program is coded in C, and is essentially a hardware description of the game. The program manipulates
various components of the DE1-SoC board such as its keys, HEX displays, private/interval timers, and VGA output
to control and display the different parts parts of the game.

![Alt text](/Lumberjack_OpeningScreen.png?raw=true "Lumberjack_OpeningScreen")
A screenshot of the opening screen of the game

![Alt text](/Lumberjack_Gameplay.png?raw=true "Lumberjack_Gameplay")
A screenshot of the game being played
