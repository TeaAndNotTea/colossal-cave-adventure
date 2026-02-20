emcc -O2 \
  --preload-file advent.dat \
  getin_shim.c advent_nosave_f2c2.c advent_common_init.c timewrap.c main_stub.c \
  -I./libf2c ./libf2c.a \
  -sMODULARIZE=1 \
  -sEXPORT_NAME='createAdventModule' \
  -sASYNCIFY \
  -sASYNCIFY_IMPORTS=['get_js_input'] \
  -sFORCE_FILESYSTEM=1 \
  --js-library library_getin.js \
  -o advent.js
