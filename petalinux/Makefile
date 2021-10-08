ROOT			     = $(PWD)
PROJECT_NAME        ?= build
PROJECT_PATH         = $(ROOT)/$(PROJECT_NAME)
K26_BSP_PATH        ?= ${ROOT}/xilinx-k26-starterkit-v2020.2.2-final.bsp
PL_VERSION          ?= "2020_2_2"

define set_cfg_value
	@sed -i 's/$(1)=.*/$(1)=$(2)/' $(3)
	@grep $(1) $(3) || echo "$(1)=$(2)" >> $(3)
endef

.PHONY: all
all: petalinux-create petalinux-config petalinux-build petalinux-package

.PHONY: check
check:
ifndef PETALINUX_VER
	$(error You have to source Petalinux settings)
endif
ifneq PYTHON3
	$(error PYTHON3 must be defined)
endif


petalinux-create: check
	@# create project
	@cd $(ROOT) && petalinux-create -t project -s $(K26_BSP_PATH) --name $(PROJECT_NAME)

petalinux-config: check petalinux-create
	@petalinux-config -p $(PROJECT_PATH) --silentconfig
	@# ============= Add accelize drm library =============
	@petalinux-create -p $(PROJECT_PATH) -t apps --template install -n libaccelize-drm --enable
	@$PYTHON3 $(ROOT)/prepare.py $(PL_VERSION) -o $(ROOT)/libaccelize-drm/
	@cp -r $(ROOT)/libaccelize-drm/libaccelize-drm.bb \
			$(PROJECT_PATH)/project-spec/meta-user/recipes-apps/libaccelize-drm/
	#@# ============= fix initramfs problem of KV260 build =============
	#@mkdir -p $(PROJECT_PATH)/project-spec/meta-user/recipes-apps/som-carrier-autoload
	#@cp -r $(ROOT)/petalinux/som-carrier-autoload/ \
	#		$(PROJECT_PATH)/project-spec/meta-user/recipes-apps

petalinux-build: check
	@petalinux-build -p $(PROJECT_PATH)
