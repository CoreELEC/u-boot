AOCPU Access Limitation (T5w) {#aocpu_access_limitation_t5w}
===============
"[ ]": can not access  
" * ": can access, but need CAPU permission
aocpu|module
:-:|:-
[ ]|rom
*|ahb sram
[ ]|mali
[ ]|usbctrl
[ ]|emmcC
[ ]|emmcB
*|async_fifo3
*|uart0
*|uart1
*|uart2
*|pwm_gh
*|ciplus_ctrl
*|i2c_m0
*|i2c_m1
*|i2c_m2
*|i2c_m3
*|pwm_ab
*|pwm_cd
*|pwm_ef
*|msr_clk
*|spicc_2
*|spicc_1
*|spifc
*|spicc_0
*|isa
*|stream
*|async_fifo
*|async_fifo2
*|assist
*|aififo
*|reset
[ ]|gic
[ ]|gpv
[ ]|ge2d
[ ]|vpu
*|sar_adc
*|ir_dec
*|i2c_s
[ ]|led_ctrl
*|rti
*|hrxcore
*|hdmirx
*|demod
*|tcon
*|usbphy22
*|atv_dmd
*|tvfe
*|au_cpu
*|reset_sec
*|eth_phy
*|adec
*|hdmi20_aes
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
*|secure_top
*|audio
[ ]|usb0
[ ]|usb1
[ ]|eth
*|ddr_ctrl
[ ]|flash
[ ]|a55_dbg
*|ddr


### Note for ddr access ###

By default, T5W AOCPU can not access DDR memory because of a register "AO_CPU_SYS_CNTL". The default value is 1 and disable the path from AOCPU to DDR.  
If AOCPU access DDR in that state, it would generate exeception: load or store access fault.  
And if you want AOCPU to access DDR, should set bit[0] of register "AO_CPU_SYS_CNTL" to 0 in tee state.  

