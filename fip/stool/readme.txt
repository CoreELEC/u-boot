IMPORTANT:
1. uboot prepare
   1.1 you must get the latest version which support script sign uboot feature first
	 1.2 then open the switch #define CONFIG_AML_SIGNED_UBOOT   1 //gxl_skt_v1.h
	 1.3 build uboot successfully with command ./mk gxl_skt_v1
   1.4 get package file for v3 secript gxl_skt_v1-u-boot.aml.zip
2. image prepare
   2.1 kernel/recovery/dtb image should be ready and place them to folder ./input
3. key prepare
   3.0 run ./key.create.bash keypath to generate keys to folder keypath
   3.1 for uboot signing you must afford RSA/AES key for bl2/bl3x and place them to folder ./key
   3.2 for kernel/recovery/dtb signing you must afford RSA/AES key and place them to folfder ./key
4. after above done then just run command
   4.1 ./sign.sh -p input -z package -r rsakey -a aeskey -o output
       4.1.1 -p input   //input folder
       4.1.2 -z zip package file //uboot package image
       4.1.3 -r frsakey //rsa key folder
       4.1.4 -a faeskey //aes key folder
       4.1.5 -o output  //output folder

FOLDER ARCHITECTURE:
|-input  //input for script signing tool, support set with -p inputfolder
|  |
|  |--bl2_new.bin  //bl2  -- must for uboot
|  |--bl30_new.bin //bl30 -- must for uboot
|  |--bl31.img     //bl31 -- must for uboot
|  |--bl33.bin     //bl33 -- must for uboot
|  |--bl32.img     //bl32 -- optional for uboot
|  |--boot.img     //boot     -- must for kernel
|  |--recovery.img //recovery -- must for recovery
|  |--dt.img       //dt       -- must for dtb
|
|-key  //input for script signing tool, support set with -r rsakeyfolder and -a aeskeyfolder
|  |
|  |--root.pem    //root RSA key  -- must for uboot,must come from root0/1/2/3
|  |--root0.pem   //root RSA key0 -- must for uboot
|  |--root1.pem   //root RSA key1 -- must for uboot
|  |--root2.pem   //root RSA key2 -- must for uboot
|  |--root3.pem   //root RSA key3 -- must for uboot
|  |--bl2.pem     //bl2 RSA key  -- must for uboot
|  |--bl2aesiv    //bl2 aes key IV -- must for uboot
|  |--bl2aeskey   //bl2 aes key  -- must for uboot
|	 |--bl3xkey.pem //bl3x RSA key  -- must for uboot
|  |--bl3xaesiv   //bl3x aes key IV -- must for uboot
|  |--bl3xaekey   //bl3x aes key  -- must for uboot
|  |--kernelkey.pem //RSA key    -- must for kernel/recovery/dtb
|  |--kernelaesiv   //aes key IV -- must for kernel/recovery/dtb
|  |--kernelaeskey  //aes key    -- must for kernel/recovery/dtb
|  |
|-output  //input for script signing tool, support set with -o outputfolder
|  |
|  |--pattern.efuse                         //EFUSE pattern for secure boot
|  |--u-boot.bin.signed.encrypted           //signed uboot for NAND/SPI/eMMC
|  |--u-boot.bin.usb.bl2.signed.encrypted   //signed BL2 for usb boot only
|  |--u-boot.bin.usb.tpl.signed.encrypted   //signed TPL for usb boot only
|  |--u-boot.bin.signed.encrypted.sd.bin    //signed uboot for SD card boot only
|  |--boot.img.encrypt                      //signed kernel image
|  |--recovery.img.encrypt                  //signed recovery image
|  |--dtb.img.encrypt                       //signed dtb image
|  |
|-signing-tool-gxl      //tool set for signing -- DO NOT MODIFY
|-signing-tool-gxl-dev  //tool set for signing -- DO NOT MODIFY
|-amlogic-sign-gxl.sh   //tool for signing GXL/TXLX -- DO NOT MODIFY
|-signing-tool-g12a     //tool set for signing -- DO NOT MODIFY
|-signing-tool-g12a-dev //tool set for signing -- DO NOT MODIFY
|-amlogic-sign-g12a.sh  //tool for signing G12A/B -- DO NOT MODIFY
|-sign.sh               //tool for signing -- DO NOT MODIFY
|-readme.txt            //it is me