#
#■この makefile の利用手引き
# 1. このファイルをサンプルのディレクトリにコピーしてください。
# 2. ディレクトリの設定を調整してください。
#    マクロ GCC, SEGALIB, SEGASMP を定義してください。
# 3. コンフィギュレーションファイル sample.cfg を作成してください。
#    マクロ TARGET と OBJS を定義してください。
# 4. make を実行してサンプルプログラムを作成してください。
#

#
#■このファイルで使われているマクロの説明
#  CC           Ｃコンパイラを指定します。（実行可能な形で記述してください）
#  CFLAGS       Ｃコンパイラのオプションを指定します。
#  AS           アセンブラを指定します。（実行可能な形で記述してください）
#  ASFLAGS      アセンブラのオプションを指定します。
#  LDFLAGS      リンカのオプションを指定します。
#  SATURN       サターン用のホームディレクトリを指定します。
#  GCC          ＧＮＵのあるディレクトリを指定します。
#  SEGASMP      ＳＢＬのサンプルのあるディレクトリを指定します。
#  SEGALIB      ＳＢＬのライブラリのあるディレクトリを指定します。
#  LOCATE_FILE  メモリのロケーションを記述したファイルです。
#  ENTRY_POINT  サンプルプログラムの実行開始アドレスを指定します。
#  LIBS         リンクするライブラリを指定します。
#  
#  （コンフィギュレーションファイル sample.cfg の中で定義します）
#  TARGET       サンプルプログラムの名前を指定します。
#               （ファイルのプライマリネームになります）
#  OBJS         リンクするオブジェクトファイルを指定します。
#

CC = sh-coff-gcc
AS = gasm

CFLAGS  = -g -O2 -Wall -std=gnu99 -m2 -DMODEL_S -I. -Isaturn -Ines -Iff1 -Iff1/sat -I$(GCC)/include -I$(SEGALIB)/include -I$(SEGASMP)/per/smpclib
#make JP=1 builds for the Japanese ROM.
ifeq ($(JP),1)
CFLAGS += -DFF1_JP
endif
ASFLAGS =
LDFLAGS = -T $(LOCATE_FILE) -e $(ENTRY_POINT) -nostartfiles

SATURN  = ../../sbl6
GCC     = c:/saturn/SH_COFF/sh-coff
SEGASMP = $(SATURN)/segasmp
SEGALIB = $(SATURN)/segalib

LOCATE_FILE = $(SEGASMP)/lib/saturn.lnk
ENTRY_POINT = START
CONFIG_FILE = sample.cfg
LIBS_FILE = $(SEGASMP)/lib/libs.lst

include	$(LIBS_FILE)

include	$(CONFIG_FILE)

.SUFFIXES:
.SUFFIXES: .cof .o .src .c

vpath %.c saturn nes ff1 ff1/sat

all: $(TARGET).cof $(TARGET).bin

$(TARGET).cof:	$(OBJS)
	$(CC) $(LDFLAGS) $(_LDFLAGS) -o $(TARGET).cof -Xlinker -Map -Xlinker $(TARGET).map $(OBJS) $(LIBS)

$(TARGET).bin:	$(OBJS)
	sh-coff-objcopy -O binary $(TARGET).cof out.bin

%.o: %.c
	$(CC) -c $(CFLAGS) $(_CFLAGS) -o $@ $<

%.o: %.src
	cmd //c $(AS) $< $(ASFLAGS) $(_ASFLAGS) -o $@



# Fails if FF1 game code (ff1/, not ff1/sat/) reaches an SBL header or a 0x25xxxxxx hardware address.
.PHONY: check
check:
	@fail=0; \
	for f in ff1/*.c; do \
		if $(CC) -MM $(CFLAGS) $$f | grep -qi "sega_"; then echo "SBL header reached: $$f"; fail=1; fi; \
	done; \
	if grep -il "include *<sega_" ff1/*.h; then fail=1; fi; \
	if grep -nE "0x25[0-9A-Fa-f]{6}" ff1/*.c ff1/*.h; then fail=1; fi; \
	if [ $$fail = 0 ]; then echo "check: ff1/ is SBL-free"; fi; \
	exit $$fail
