#!/bin/bash
mkdir -p generate-binaries/data/payload
mkdir -p generate-binaries/data/template/chipset
cp fip/_tmp/bl30-payload.bin generate-binaries/data/payload
cp fip/_tmp/bl33-payload.bin generate-binaries/data/payload
cp fip/_tmp/dvinit-params.bin generate-binaries/data/payload
cp fip/_tmp/bb1st.sto.bin.signed generate-binaries/data/template/chipset/bb1st.sto.bin
cp fip/_tmp/bb1st.usb.bin.signed generate-binaries/data/template/chipset/bb1st.usb.bin
cp fip/_tmp/blob-bl2e.sto.bin.signed generate-binaries/data/template/chipset/blob-bl2e.sto.bin
cp fip/_tmp/blob-bl2e.usb.bin.signed generate-binaries/data/template/chipset/blob-bl2e.usb.bin
cp fip/_tmp/blob-bl2x.bin.signed generate-binaries/data/template/chipset/blob-bl2x.bin
cp fip/_tmp/blob-bl31.bin.signed generate-binaries/data/template/chipset/blob-bl31.bin
cp fip/_tmp/blob-bl32.bin.signed generate-binaries/data/template/chipset/blob-bl32.bin
cp fip/_tmp/blob-bl40.bin.signed generate-binaries/data/template/chipset/blob-bl40.bin
cp fip/_tmp/*ddr-fip.* generate-binaries/data/template/chipset/
tar zcvf bootloader.tar.gz generate-binaries
rm -rf generate-binaries

