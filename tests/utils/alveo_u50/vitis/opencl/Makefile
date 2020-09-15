VPP := $(XILINX_VITIS)/bin/v++
EMCONFIGUTIL := $(XILINX_VITIS)/bin/emconfigutil
MODE := hw
#DSA := xilinx_u200_qdma_201910_1
DSA := xilinx_u50_gen3x16_xdma_201920_3

# sources
HOST_SRC := src/host_ocl.cpp
KRNL_SRC := kernel.cpp

# kernel targets
KRNL_XO := kernel.$(MODE).xo
XCLBIN := kernel.$(MODE).xclbin

# host target
HOST_EXE := host.exe

# config files target
EMCONFIG_FILE := emconfig.json

VPP_OPTS := -s -t $(MODE) --platform $(DSA)
VPP_OPTS += --temp_dir ./all_builds/$(TARGET)
VPP_OPTS += --report_dir ./all_logs/$(TARGET)
VPP_OPTS += --log_dir ./all_logs/$(TARGET)

CFLAGS := -g -std=c++11 -I$(XILINX_XRT)/include -I/usr/include
LFLAGS := -L$(XILINX_XRT)/lib -lxilinxopencl -lxrt_core -lrt -luuid -L/usr/lib -laccelize_drmc

# run time args
SUM := 5
EXE_OPT := kernel.${MODE}.xclbin ${SUM}

# primary build targets
.PHONY: xclbin app all

xclbin:  $(XCLBIN)
app: $(HOST_EXE)

all: xclbin app

clean:
	-$(RM) $(EMCONFIG_FILE) $(HOST_EXE) $(XCLBIN)

# kernel rules
$(KRNL_XO): $(KRNL_SRC)
	$(RM) $@
	$(VPP) $(VPP_OPTS) -c -k increment -o $@ $+


$(XCLBIN): $(KRNL_XO)
	$(VPP) $(VPP_OPTS) -l -o $@ $(KRNL_XO)

# host rules
$(HOST_EXE): $(HOST_SRC)
	g++ $(CFLAGS) -o $@ $+ $(LFLAGS)
	@echo 'Compiled Host Executable: $(HOST_EXE)'

$(EMCONFIG_FILE):
	$(EMCONFIGUTIL) --nd 1 --od . --platform $(DSA)

check: $(XCLBIN) $(HOST_EXE) $(EMCONFIG_FILE)
	XCL_EMULATION_MODE=${MODE} ./$(HOST_EXE) $(EXE_OPT)
