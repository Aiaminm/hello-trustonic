# mytl — build a trustlet and run it on a device
#
#   make            build the trustlet (mytl.tlbin)
#   make client     build the normal-world client
#   make test       deploy + run the client               [device + root]
#   make unmount    tear down the on-device overlay
#   make check      compile-check the client
#   make clean      remove build artifacts
#
# Overrides:
#   CROSS          AArch64 toolchain prefix        (aarch64-linux-gnu)
#   NDK            Android NDK root                ($ANDROID_NDK_HOME)
#   NDK_HOST       NDK prebuilt host tag           (from uname)
#   API            Android API level               (34)
#   TEE_USERSPACE  trustonic-tee-user-space tree   (bundled submodule)
#   ADB            adb binary                      (adb)
#   UUID           trustlet UUID

# ---- trustlet toolchain ----------------------------------------------------
CROSS     ?= aarch64-linux-gnu
TA_CC      = $(CROSS)-gcc
TA_LD      = $(CROSS)-ld
TA_CFLAGS  = -ffreestanding -nostdlib -fno-pic -fno-pie -fno-stack-protector \
             -fno-asynchronous-unwind-tables -mbranch-protection=none \
             -march=armv8-a -Os -Wall -Wextra
TA_SRC     = trustlet
TOOLS      = tools
TA_OBJ     = $(TA_SRC)/start.o $(TA_SRC)/tlmain.o
TLBIN      = mytl.tlbin

# ---- client toolchain (Android NDK) ----------------------------------------
NDK       ?= $(ANDROID_NDK_HOME)
API       ?= 34
NDK_HOST  ?= $(shell uname -s | tr A-Z a-z)-x86_64
NDK_CC     = $(NDK)/toolchains/llvm/prebuilt/$(NDK_HOST)/bin/aarch64-linux-android$(API)-clang
TEE_USERSPACE ?= third_party/trustonic-tee-user-space
CLIENT_INC = -I$(TEE_USERSPACE)/MobiCoreDriverLib/ClientLib/public -I$(TEE_USERSPACE)/common/MobiCore/inc
CLIENT     = client/mytl_client

# ---- device ----------------------------------------------------------------
ADB       ?= adb
UUID      ?= 0a0a0a0a000000000000000000000001
SU         = $(ADB) shell su -c
DEV_TMP    = /data/local/tmp
OVL        = $(DEV_TMP)/ovl
REG        = /vendor/app/mcRegistry
LABEL      = u:object_r:mobicore_vendor_file:s0

# ----------------------------------------------------------------------------
.PHONY: all client test deploy unmount check clean

all: $(TLBIN)

$(TA_SRC)/%.o: $(TA_SRC)/%.S
	$(TA_CC) $(TA_CFLAGS) -c $< -o $@

$(TA_SRC)/%.o: $(TA_SRC)/%.c
	$(TA_CC) $(TA_CFLAGS) -c $< -o $@

$(TA_SRC)/mytl.elf: $(TA_OBJ) $(TA_SRC)/mytl.ld
	$(TA_LD) -T $(TA_SRC)/mytl.ld -static --no-dynamic-linker $(TA_OBJ) -o $@

$(TLBIN): $(TA_SRC)/mytl.elf $(TOOLS)/elf2mclf.py
	python3 $(TOOLS)/elf2mclf.py $< --uuid $(UUID) -o $@

client: $(CLIENT)

$(TEE_USERSPACE)/MobiCoreDriverLib/ClientLib/public/MobiCoreDriverApi.h:
	@echo "headers missing at $(TEE_USERSPACE); run: git submodule update --init" >&2
	@exit 1

client/libMcClient.so:
	$(ADB) exec-out su -c "cat /vendor/lib64/libMcClient.so" > $@

$(CLIENT): client/mytl_client.c client/libMcClient.so $(TEE_USERSPACE)/MobiCoreDriverLib/ClientLib/public/MobiCoreDriverApi.h
	$(NDK_CC) -Wall -O2 $(CLIENT_INC) $< -Lclient -lMcClient -o $@

check: client/mytl_client.c $(TEE_USERSPACE)/MobiCoreDriverLib/ClientLib/public/MobiCoreDriverApi.h
	$(NDK_CC) -Wall -O2 $(CLIENT_INC) -c $< -o /dev/null && echo "client headers OK"

# overlay the trustlet into the read-only registry (idempotent)
deploy: $(TLBIN)
	-$(SU) "umount $(REG)"
	-$(SU) "umount $(OVL)"
	$(SU) "mkdir -p $(OVL)"
	$(SU) "mount -t tmpfs tmpfs $(OVL)"
	$(SU) "mkdir $(OVL)/up $(OVL)/work"
	$(ADB) push $(TLBIN) $(DEV_TMP)/$(UUID).tlbin
	$(SU) "cp $(DEV_TMP)/$(UUID).tlbin $(OVL)/up/$(UUID).tlbin"
	$(SU) "chcon $(LABEL) $(OVL)/up/$(UUID).tlbin"
	$(SU) "mount -t overlay overlay -o lowerdir=$(REG),upperdir=$(OVL)/up,workdir=$(OVL)/work $(REG)"

# build, deploy, run
test: deploy $(CLIENT)
	$(ADB) push $(CLIENT) $(DEV_TMP)/mytl_client
	$(SU) "chmod 755 $(DEV_TMP)/mytl_client"
	$(SU) "LD_LIBRARY_PATH=/vendor/lib64 $(DEV_TMP)/mytl_client"

unmount:
	-$(SU) "umount $(REG)"
	-$(SU) "umount $(OVL)"

clean:
	rm -f $(TA_OBJ) $(TA_SRC)/mytl.elf $(TLBIN) $(CLIENT)
