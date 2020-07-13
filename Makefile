.PHONY: install build help
.DEFAULT_GOAL   = help

PRIMARY_COLOR   		= \033[0;34m
PRIMARY_COLOR_BOLD   	= \033[1;34m
SUCCESS_COLOR   		= \033[0;32m
SUCCESS_COLOR_BOLD   	= \033[1;32m
DANGER_COLOR    		= \033[0;31m
DANGER_COLOR_BOLD    	= \033[1;31m
WARNING_COLOR   		= \033[0;33m
WARNING_COLOR_BOLD   	= \033[1;33m
NO_COLOR      			= \033[m

BUILD_DIR				?= /home/centos

install: ## Install Sphinx
	# We must check if installation is already done before running those commands
	yum install python3-pip -y
	pip3 install --user sphinx
	pip3 install --user sphinx_rtd_theme

build: ## Build the doc into "BUILD_DIR" (= /home/centos if not defined)
	sphinx-build -b html ./doc $(BUILD_DIR)

help: ## Display this help
	@awk 'BEGIN {FS = ":.*##"; } /^[a-zA-Z_-]+:.*?##/ { printf "$(PRIMARY_COLOR_BOLD)%-10s$(NO_COLOR) %s\n", $$1, $$2 }' $(MAKEFILE_LIST) | sort
