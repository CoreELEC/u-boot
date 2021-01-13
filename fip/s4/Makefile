
#
# Rules
#

DEVICE_SCS_KEY_TOP ?= $(CURDIR)/keys/dev-keys/sc2/device/
DEVICE_ROOTRSA_INDEX ?= 0
PROJECT ?= s905x4
DEVICE_SCS_SEGID ?= 0x0
DEVICE_VENDOR_SEGID ?= 0x0
DEVICE_SCS_VERS ?= 0x0
DEVICE_TEE_VERS ?= 0x0
DEVICE_REE_VERS ?= 0x0

ARGS += "DEVICE_SCS_KEY_TOP=$(DEVICE_SCS_KEY_TOP)"
ARGS += "DEVICE_ROOTRSA_INDEX=$(DEVICE_ROOTRSA_INDEX)"
ARGS += "PROJECT=$(PROJECT)"
ARGS += "DEVICE_BUILD_PATH=${DEVICE_BUILD_PATH}"
ARGS += "DEVICE_SCS_SEGID=${DEVICE_SCS_SEGID}"
ARGS += "DEVICE_VENDOR_SEGID=${DEVICE_VENDOR_SEGID}"
ARGS += "DEVICE_SCS_VERS=${DEVICE_SCS_VERS}"
ARGS += "DEVICE_TEE_VERS=${DEVICE_TEE_VERS}"
ARGS += "DEVICE_REE_VERS=${DEVICE_REE_VERS}"

all: build

dv-template:
	make -C create-template/run $(ARGS)

dv-sign:
	make -C generate-binaires/run $(ARGS)

build:
	make -C create-template/run $(ARGS)
	make -C generate-binaires/run $(ARGS)

clean:
	make clean -C create-template/run
	make clean -C generate-binaires/run
