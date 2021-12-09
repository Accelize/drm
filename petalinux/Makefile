ROOT			     = $(PWD)
PROJECT_NAME        ?= build
PROJECT_PATH         = $(ROOT)/$(PROJECT_NAME)
PYTHON3             ?= /usr/bin/python3
PKG_DIR             := rpms

SELF_DIR            := $(dir $(lastword $(MAKEFILE_LIST)))

.PHONY: all create config build clean

all: create config build

.PHONY: check
check:
ifndef PETALINUX_VER
	$(error You have to source Petalinux settings)
endif
ifndef K26_BSP_PATH
	$(error You have to define K26_BSP_PATH with the path to BSP)
endif

all: pkg

pkg: $(wildcard $(PROJECT_PATH)/build/tmp/deploy/rpm/cortexa72_cortexa53/libaccelize-drm*.rpm)
	@echo Generate rpms
	@mkdir -p $(PKG_DIR)
	@cp $(PROJECT_PATH)/build/tmp/deploy/rpm/cortexa72_cortexa53/libaccelize-drm*.rpm $(PKG_DIR)
	@tar -czf libaccelize-drm.tgz $(PKG_DIR)

%.rpm: build

build: $(PROJECT_PATH)/project-spec/meta-user/recipes-apps/libaccelize-drm/libaccelize-drm.bb
	@echo Build petalinux project
	@petalinux-build -p $(PROJECT_PATH)

libaccelize-drm.bb: config

config: | $(PROJECT_PATH)/config.project
	@echo Add libaccelize-drm recipe
	@petalinux-create -p $(PROJECT_PATH) -t apps --template install -n libaccelize-drm --enable --force
	@$(PYTHON3) $(SELF_DIR)/prepare.py $(PETALINUX_VER) -o $(PROJECT_PATH)/project-spec/meta-user/recipes-apps/libaccelize-drm/
	
$(PROJECT_PATH)/config.project: create

create: check
	@echo Create petalinux project
	@# create project
	@cd $(ROOT) && petalinux-create -t project -s $(K26_BSP_PATH) --name $(PROJECT_NAME)
	@petalinux-config -p $(PROJECT_PATH) --silentconfig
	
clean:
	@rm -rf $(PROJECT_PATH)
