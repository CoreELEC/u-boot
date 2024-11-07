#!/bin/bash
# This is just a demo batch command, it's used to rename and copy bootloader binary files from ~/uboot/fip/_tmp/ to the new directory that's used by signing tools
mkdir -p generate-binaries/data/payload
mkdir -p generate-binaries/data/template/chipset
mkdir -p create-template/data/template/chipset
cp ./soc/templates/s1a/$1/bb1st.bin create-template/data/template/chipset/bb1st.sto.bin
cp ./soc/templates/s1a/$1/bb1st.bin create-template/data/template/chipset/bb1st.usb.bin
cp ./soc/templates/s1a/$1/device-fip-header.bin create-template/data/template/chipset/
cp fip/_tmp/bl30-payload.bin generate-binaries/data/payload
cp fip/_tmp/bl33-payload.bin generate-binaries/data/payload
cp fip/_tmp/dvinit-params.bin generate-binaries/data/payload
cp fip/_tmp/bb1st.sto.bin.signed generate-binaries/data/template/chipset/bb1st.sto.bin.signed
cp fip/_tmp/bb1st.usb.bin.signed generate-binaries/data/template/chipset/bb1st.usb.bin.signed
cp fip/_tmp/blob-bl2e.sto.bin.signed generate-binaries/data/template/chipset/blob-bl2e.sto.bin
cp fip/_tmp/blob-bl2e.usb.bin.signed generate-binaries/data/template/chipset/blob-bl2e.usb.bin
cp fip/_tmp/blob-bl2x.bin.signed generate-binaries/data/template/chipset/blob-bl2x.bin
cp fip/_tmp/blob-bl31.bin.signed generate-binaries/data/template/chipset/blob-bl31.bin
cp fip/_tmp/blob-bl32.bin.signed generate-binaries/data/template/chipset/blob-bl32.bin
cp fip/_tmp/blob-bl40.bin.signed generate-binaries/data/template/chipset/blob-bl40.bin
cp fip/_tmp/ddr-fip.bin generate-binaries/data/template/chipset/ddr-fip.bin
tar zcvf bootloader.tar.gz generate-binaries create-template
rm -rf generate-binaries
rm -rf create-template

