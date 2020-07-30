
#
# Rules
#

DEVICE_SCS_KEY_TOP ?= $(CURDIR)/keys/dev-keys/sc2/device/
DEVICE_ROOTRSA_INDEX ?= 0
PROJECT ?= s905x4

ARGS += "DEVICE_SCS_KEY_TOP=$(DEVICE_SCS_KEY_TOP)"
ARGS += "DEVICE_ROOTRSA_INDEX=$(DEVICE_ROOTRSA_INDEX)"
ARGS += "PROJECT=$(PROJECT)"
ARGS += "DEVICE_BUILD_PATH=${DEVICE_BUILD_PATH}"

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
