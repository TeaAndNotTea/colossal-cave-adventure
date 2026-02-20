# -----------------------------------------
# Colossal Adventure plugin build + deploy
# -----------------------------------------

PLUGIN_SLUG 	:= colossal-adventure
# Plugin is constructed here
DIST_DIR    	:= dist/$(PLUGIN_SLUG)
# Emscripten build directory
BUILD_DIR   	:= build
# JS Sources
SRC_JS			:= src/js
# C Sources (outputs of f2c and C shims)
SRC_C			:= src/advent_C
# C Libraries (just libf2c for now)
LIB_C			:= libs_C
# Adventure data lives here
ADVENT_DATA 	:= advent
# Image paths
ROOM_IMG_SRC	:= assets/rooms
ITEM_IMG_SRC	:= assets/items
# Plugin skeleton. Contains the PHP plugin wrapper and title image
SKEL        	:= plugin_skel

# Remote dev server + docker/wp settings
REMOTE_HOST 		:= dave@192.168.1.202
REMOTE_PLUGINS_DIR 	:= /home/dave/docker/wp-data/wp-content/plugins
WP_CONTAINER 		:= docker-wordpress-1
WP_PATH 			:= /var/www/html

# Source files compiled into WASM
WASM_SRCS := \
	$(SRC_C)/getin_shim.c \
	$(SRC_C)/advent_nosave_f2c2.c \
	$(SRC_C)/advent_common_init.c \
	$(SRC_C)/timewrap.c \
	$(SRC_C)/main_stub.c

# Emscripten build flags
EMCC_FLAGS := -O2 \
  --preload-file advent.dat \
  -I./src/libf2c $(LIB_C)/libf2c.a \
  -sMODULARIZE=1 \
  -sEXPORT_NAME='createAdventModule' \
  -sASYNCIFY \
  -sASYNCIFY_IMPORTS='["get_js_input"]' \
  -sFORCE_FILESYSTEM=1 \
  --js-library $(SRC_JS)/library_getin.js

.PHONY: all clean wasm images assemble zip deploy

all: assemble

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR) dist/$(PLUGIN_SLUG).zip

# ------------
# Build WASM
# ------------

wasm: $(BUILD_DIR)/advent.js

$(BUILD_DIR)/advent.js: $(WASM_SRCS) $(ADVENT_DATA)/advent.dat $(LIB_C)/libf2c.a $(SRC_JS)/library_getin.js
	mkdir -p $(BUILD_DIR)
	emcc $(WASM_SRCS) -o $(BUILD_DIR)/advent.js $(EMCC_FLAGS)

# ----------------------------------
# Assemble WordPress plugin folder
# ----------------------------------

images:
	mkdir -p $(DIST_DIR)/assets/rooms
	rsync -a --delete $(ROOM_IMG_SRC)/ $(DIST_DIR)/assets/rooms/
	mkdir -p $(DIST_DIR)/assets/items
	rsync -a --delete $(ITEM_IMG_SRC)/ $(DIST_DIR)/assets/items/

assemble: wasm
	rm -rf $(DIST_DIR)
	mkdir -p $(DIST_DIR)

	# copy WP plugin skeleton (PHP wrapper, etc.)
	rsync -a $(SKEL)/ $(DIST_DIR)/

	# copy wasm/js artifacts
	mkdir -p $(DIST_DIR)/assets
	rsync -a $(BUILD_DIR)/ $(DIST_DIR)/assets/

	# JS wrappers
	rsync -a $(SRC_JS)/ $(DIST_DIR)/assets/

	# Room images
	$(MAKE) images

zip: assemble
	cd dist && zip -r $(PLUGIN_SLUG).zip $(PLUGIN_SLUG)

# -------------------------------------------------
# Deploy to dev server (WP in Docker)
# - deactivate plugin
# - rsync dist -> host-mounted wp-content/plugins
# - chown + activate plugin
# -------------------------------------------------

SSH := ssh -tt

deploy: assemble
	$(SSH) $(REMOTE_HOST) "sudo docker exec -u 0 $(WP_CONTAINER) sh -lc 'set -e; cd $(WP_PATH); wp plugin deactivate $(PLUGIN_SLUG) --allow-root || true'"
	rsync -av --delete $(DIST_DIR)/ $(REMOTE_HOST):$(REMOTE_PLUGINS_DIR)/$(PLUGIN_SLUG)/
	$(SSH) $(REMOTE_HOST) "sudo docker exec -u 0 $(WP_CONTAINER) sh -lc 'set -e; chown -R www-data:www-data $(WP_PATH)/wp-content/plugins/$(PLUGIN_SLUG); cd $(WP_PATH); wp plugin activate $(PLUGIN_SLUG) --allow-root'"

