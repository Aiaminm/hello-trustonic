# mytl — from-scratch Kinibi trustlet: build + on-device test
#
#   make            build the trustlet (mytl.tlbin)            [no device needed]
#   make client     build the normal-world client             [needs device: pulls libMcClient.so]
#   make test       mount overlay + push + run the client      [needs a Kinibi patched to accept
#                                                                self-signed trustlets, + root]
#   make unmount    tear down the on-device overlay
#   make check      compile-check the client (headers only)    [no device, no lib]
#   make clean      remove build artifacts
#
# Overridable (all have sane defaults; set on the command line or as env vars):
#   CROSS          bare-metal AArch64 toolchain prefix          (default aarch64-linux-gnu)
#   NDK            Android NDK root                             (default $ANDROID_NDK_HOME)
#   NDK_HOST       NDK prebuilt host tag                        (default from `uname`)
#   API            Android API level for the client            (default 34)
#   TEE_USERSPACE  Trustonic trustonic-tee-user-space headers   (default: the bundled submodule)
#   ADB            adb binary                                   (default: adb on PATH)
#   UUID           trustlet UUID                                (default 0a0a0a0a…0001)

# ---- trustlet toolchain ----------------------------------------------------
CROSS     ?= aarch64-linux-gnu
TA_CC      = $(CROSS)-gcc
TA_LD      = $(CROSS)-ld
TA_CFLAGS  = -ffreestanding -nostdlib -fno-pic -fno-pie -fno-stack-protector \
             -fno-asynchronous-unwind-tables -mbranch-protection=none \
             -march=armv8-a -Os -Wall -Wextra
TLBIN      = mytl.tlbin

# ---- client toolchain (Android NDK) ----------------------------------------
# NDK defaults to $ANDROID_NDK_HOME; override with `make NDK=/path/to/ndk`.
# NDK_HOST is the prebuilt host tag (linux-x86_64 / darwin-x86_64), derived from uname.
NDK       ?= $(ANDROID_NDK_HOME)
API       ?= 34
NDK_HOST  ?= $(shell uname -s | tr A-Z a-z)-x86_64
NDK_CC     = $(NDK)/toolchains/llvm/prebuilt/$(NDK_HOST)/bin/aarch64-linux-android$(API)-clang
# Trustonic client headers (ClientLib public API + MobiCore inc), vendored as a
# submodule. If the dir is empty after cloning, run:
#   git submodule update --init
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

start.o: start.S
	$(TA_CC) $(TA_CFLAGS) -c $< -o $@

tlmain.o: tlmain.c
	$(TA_CC) $(TA_CFLAGS) -c $< -o $@

mytl.elf: start.o tlmain.o mytl.ld
	$(TA_LD) -T mytl.ld -static --no-dynamic-linker start.o tlmain.o -o $@

$(TLBIN): mytl.elf elf2mclf.py
	python3 elf2mclf.py mytl.elf --uuid $(UUID) -o $@

# client (links against the device's libMcClient.so, pulled on demand)
client: $(CLIENT)

# fail early with a clear hint if the headers submodule hasn't been initialised
$(TEE_USERSPACE)/MobiCoreDriverLib/ClientLib/public/MobiCoreDriverApi.h:
	@echo "Trustonic headers missing at $(TEE_USERSPACE)." >&2
	@echo "Run: git submodule update --init" >&2
	@exit 1

client/libMcClient.so:
	$(ADB) exec-out su -c "cat /vendor/lib64/libMcClient.so" > $@

$(CLIENT): client/mytl_client.c client/libMcClient.so $(TEE_USERSPACE)/MobiCoreDriverLib/ClientLib/public/MobiCoreDriverApi.h
	$(NDK_CC) -Wall -O2 $(CLIENT_INC) $< -Lclient -lMcClient -o $@

check: client/mytl_client.c $(TEE_USERSPACE)/MobiCoreDriverLib/ClientLib/public/MobiCoreDriverApi.h
	$(NDK_CC) -Wall -O2 $(CLIENT_INC) -c $< -o /dev/null && echo "client headers OK"

# (re)mount a tmpfs-upper overlay that drops our TA into the RO registry, then
# refresh the TA file. Idempotent: tears down any previous mount first.
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

# full on-device test: build, deploy, push + run the client
test: deploy $(CLIENT)
	$(ADB) push $(CLIENT) $(DEV_TMP)/mytl_client
	$(SU) "chmod 755 $(DEV_TMP)/mytl_client"
	$(SU) "LD_LIBRARY_PATH=/vendor/lib64 $(DEV_TMP)/mytl_client"

unmount:
	-$(SU) "umount $(REG)"
	-$(SU) "umount $(OVL)"

clean:
	rm -f start.o tlmain.o mytl.elf $(TLBIN) $(CLIENT)
