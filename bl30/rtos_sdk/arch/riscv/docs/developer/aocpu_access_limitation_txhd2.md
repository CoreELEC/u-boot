AOCPU Access Limitation (TXHD2) {#aocpu_access_limitation_txhd2}
===============
"[ ]": can not access  
" * ": can access, but need CAPU permission
aocpu|module
:-:|:-
[ ]|a55_rom
*|ahb sram
[ ]|mali
[ ]|usbctrl
[ ]|emmcC
[ ]|u2h
*|async_fifo3
*|uart0
*|uart1
*|uart2
*|i2c_m0
*|i2c_m1
*|i2c_m2
*|i2c_m3
*|pwm_ab
*|pwm_cd
*|pwm_ef
*|msr_clk
*|spifc
*|spicc_0
*|isa
*|parser
*|parser1
*|stream
*|async_fifo
*|async_fifo2
*|assist
*|stb
*|aififo
*|reset
[ ]|gic
[ ]|gpv
[ ]|ge2d
[ ]|vpu
*|sar_adc
*|ir_dec
*|pwm_ab
*|i2c_s
*|i2c_m
*|uart2
*|uart
*|pwm_cd
[ ]|led_ctrl
*|rti
*|u2drdx0
*|hdmirx_core
*|demod
*|tcon
*|hdmi20_aes
*|usbphy22
*|atv_dmd
*|tvfe
*|eth_phy
*|eth_mac
*|reset_sec
*|dsihost
*|adec
*|mipi_dsi_phy
*|clk_ctrl
*|pwr_ctrl
*|rsa
*|dma
*|hiu
*|usbphy21
*|dmc
*|usbphy20
*|ts_pll
*|periphs_reg
*|acodec
*|efuse
*|dos
*|hdmirx
*|audio
[ ]|usb0
[ ]|usb1
[ ]|eth
*|ddr_ctrl
[ ]|flash
[ ]|a55_dbg
*|ddr


### Note for ddr access ###

By default, TXHD2 AOCPU can not access DDR memory because of a register "AO_CPU_SYS_CNTL". The default value is 1 and disable the path from AOCPU to DDR.  
If AOCPU access DDR in that state, it would generate exeception: load or store access fault.  
And if you want AOCPU to access DDR, should set bit[0] of register "AO_CPU_SYS_CNTL" to 0 in tee state.  

