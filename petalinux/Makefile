ROOT			     = $(PWD)
PROJECT_NAME        ?= build
PROJECT_PATH         = $(ROOT)/$(PROJECT_NAME)
K26_BSP_PATH        ?= ${ROOT}/xilinx-k26-starterkit-v2020.2.2-final.bsp
PYTHON3             ?= /usr/bin/python3

SELF_DIR            := $(dir $(lastword $(MAKEFILE_LIST)))

.PHONY: all create config build clean

all: create config build

.PHONY: check
check:
ifndef PETALINUX_VER
	$(error You have to source Petalinux settings)
endif

create: check
	@# create project
	@cd $(ROOT) && petalinux-create -t project -s $(K26_BSP_PATH) --name $(PROJECT_NAME)
	@petalinux-config -p $(PROJECT_PATH) --silentconfig

config: check
	@# ============= Add accelize drm library =============
	@petalinux-create -p $(PROJECT_PATH) -t apps --template install -n libaccelize-drm --enable --force
	@$(PYTHON3) $(SELF_DIR)/prepare.py $(PETALINUX_VER) -o $(PROJECT_PATH)/project-spec/meta-user/recipes-apps/libaccelize-drm/
	#@# ============= fix initramfs problem of KV260 build =============
	#@mkdir -p $(PROJECT_PATH)/project-spec/meta-user/recipes-apps/som-carrier-autoload
	#@cp -r $(ROOT)/petalinux/som-carrier-autoload/ \
	#		$(PROJECT_PATH)/project-spec/meta-user/recipes-apps

build: check
	@petalinux-build -p $(PROJECT_PATH)


clean:
	@rm -rf $(PROJECT_PATH)
