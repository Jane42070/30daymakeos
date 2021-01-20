OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj \
				init.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj \
				timer.obj mtask.obj terminal.obj file.obj window.obj

OBJS_API =	api001.obj api002.obj api003.obj api004.obj api005.obj api006.obj \
			api007.obj api008.obj api009.obj api010.obj api011.obj api012.obj \
			api013.obj api014.obj api015.obj api016.obj api017.obj api018.obj \
			api019.obj api020.obj

TOOLPATH = ../tolset/z_tools/
INCPATH  = ../tolset/z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe

COPY     = cp
DEL      = rm

# 默认动作

default :
	$(MAKE) img

# 镜像文件生成

ipl10.bin : ipl10.asm Makefile
	$(NASK) ipl10.asm ipl10.bin ipl10.lst

asmhead.bin : asmhead.asm Makefile
	$(NASK) asmhead.asm asmhead.bin asmhead.lst

hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

# 操作系统
bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

apilib.lib : Makefile $(OBJS_API)
	$(GOLIB) $(OBJS_API) out:apilib.lib

# 汇编应用程序

crack7.bim : crack7.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:crack7.bim stack:1k map:crack7.map crack7.obj

crack7.hrb : crack7.bim Makefile
	$(BIM2HRB) crack7.bim crack7.hrb 0k

hello5.bim : hello5.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello5.bim stack:1k map:hello5.map hello5.obj

hello5.hrb : hello5.bim Makefile
	$(BIM2HRB) hello5.bim hello5.hrb 0

# C 语言应用程序
a.bim : a.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:a.bim stack:1k map:a.map a.obj apilib.lib

a.hrb : a.bim Makefile
	$(BIM2HRB) a.bim a.hrb 0

hello3.bim : hello3.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello3.bim stack:1k map:hello3.map hello3.obj apilib.lib

hello3.hrb : hello3.bim Makefile
	$(BIM2HRB) hello3.bim hello3.hrb 0

hello4.bim : hello4.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello4.bim stack:1k map:hello4.map hello4.obj apilib.lib

hello4.hrb : hello4.bim Makefile
	$(BIM2HRB) hello4.bim hello4.hrb 0

crack1.bim : crack1.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:crack1.bim stack:1k map:crack1.map crack1.obj apilib.lib

crack1.hrb : crack1.bim Makefile
	$(BIM2HRB) crack1.bim crack1.hrb 0

bug1.bim : bug1.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bug1.bim stack:1k map:bug1.map bug1.obj apilib.lib

bug1.hrb : bug1.bim Makefile
	$(BIM2HRB) bug1.bim bug1.hrb 0

bug2.bim : bug2.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bug2.bim stack:1k map:bug2.map bug2.obj apilib.lib

bug2.hrb : bug2.bim Makefile
	$(BIM2HRB) bug2.bim bug2.hrb 0

bug3.bim : bug3.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bug3.bim stack:1k map:bug3.map bug3.obj apilib.lib

bug3.hrb : bug3.bim Makefile
	$(BIM2HRB) bug3.bim bug3.hrb 0

winhelo.bim : winhelo.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo.bim stack:1k map:winhelo.map winhelo.obj apilib.lib

winhelo.hrb : winhelo.bim Makefile
	$(BIM2HRB) winhelo.bim winhelo.hrb 0

winhelo2.bim : winhelo2.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo2.bim stack:1k map:winhelo2.map winhelo2.obj apilib.lib

winhelo2.hrb : winhelo2.bim Makefile
	$(BIM2HRB) winhelo2.bim winhelo2.hrb 0

winhelo3.bim : winhelo3.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo3.bim stack:1k map:winhelo3.map \
		winhelo3.obj apilib.lib

winhelo3.hrb : winhelo3.bim Makefile
	$(BIM2HRB) winhelo3.bim winhelo3.hrb 40k

star1.bim : star1.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:star1.bim stack:1k map:star1.map \
		star1.obj apilib.lib

star1.hrb : star1.bim Makefile
	$(BIM2HRB) star1.bim star1.hrb 47k

star2.bim : star2.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:star2.bim stack:1k map:star2.map \
		star2.obj apilib.lib

star2.hrb : star2.bim Makefile
	$(BIM2HRB) star2.bim star2.hrb 47k

lines.bim : lines.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:lines.bim stack:1k map:lines.map \
		lines.obj apilib.lib

lines.hrb : lines.bim Makefile
	$(BIM2HRB) lines.bim lines.hrb 48k

walk.bim : walk.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:walk.bim stack:1k map:walk.map \
		walk.obj apilib.lib

walk.hrb : walk.bim Makefile
	$(BIM2HRB) walk.bim walk.hrb 48k

noodle.bim : noodle.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:noodle.bim stack:1k map:noodle.map \
		noodle.obj apilib.lib

noodle.hrb : noodle.bim Makefile
	$(BIM2HRB) noodle.bim noodle.hrb 40k

beepdown.bim : beepdown.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:beepdown.bim stack:1k map:beepdown.map \
		beepdown.obj apilib.lib

beepdown.hrb : beepdown.bim Makefile
	$(BIM2HRB) beepdown.bim beepdown.hrb 40k

beepup.bim : beepup.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:beepup.bim stack:1k map:beepup.map \
		beepup.obj apilib.lib

beepup.hrb : beepup.bim Makefile
	$(BIM2HRB) beepup.bim beepup.hrb 40k

color.bim : color.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:color.bim stack:1k map:color.map \
		color.obj apilib.lib

color.hrb : color.bim Makefile
	$(BIM2HRB) color.bim color.hrb 56k

color2.bim : color2.obj apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:color2.bim stack:1k map:color2.map \
		color2.obj apilib.lib

color2.hrb : color2.bim Makefile
	$(BIM2HRB) color2.bim color2.hrb 56k
spark.img : ipl10.bin haribote.sys Makefile \
	hello3.hrb hello4.hrb hello5.hrb a.hrb \
	crack7.hrb \
	bug1.hrb bug2.hrb bug3.hrb \
	winhelo.hrb winhelo2.hrb winhelo3.hrb \
	star1.hrb star2.hrb \
	lines.hrb walk.hrb noodle.hrb \
	beepdown.hrb beepup.hrb \
	color.hrb color2.hrb
	$(EDIMG)   imgin:../tolset/z_tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
		copy from:haribote.sys to:@: \
		copy from:ipl10.asm to:@: \
		copy from:make.bat to:@: \
		copy from:hello3.hrb to:@: \
		copy from:hello4.hrb to:@: \
		copy from:hello5.hrb to:@: \
		copy from:a.hrb to:@: \
		copy from:crack7.hrb to:@: \
		copy from:bug1.hrb to:@: \
		copy from:bug2.hrb to:@: \
		copy from:bug3.hrb to:@: \
		copy from:winhelo.hrb to:@: \
		copy from:winhelo2.hrb to:@: \
		copy from:winhelo3.hrb to:@: \
		copy from:star1.hrb to:@: \
		copy from:star2.hrb to:@: \
		copy from:lines.hrb to:@: \
		copy from:walk.hrb to:@: \
		copy from:noodle.hrb to:@: \
		copy from:beepdown.hrb to:@: \
		copy from:beepup.hrb to:@: \
		copy from:color.hrb to:@: \
		copy from:color2.hrb to:@: \
		imgout:spark.img

# 多文件联编
%.gas : %.c Makefile
	$(CC1) -o $*.gas $*.c

%.asm : %.gas Makefile
	$(GAS2NASK) $*.gas $*.asm

%.obj : %.asm Makefile
	$(NASK) $*.asm $*.obj $*.lst

# 其他指令

img :
	$(MAKE) spark.img
	$(MAKE) clean

# run :
#     $(MAKE) img
#     $(COPY) spark.img ..\tolset\z_tools\qemu\fdimage0.bin
#     $(MAKE) -C ../tolset/z_tools/qemu

run :
	../tolset/z_tools/make.exe img
	qemu-system-x86_64 -m 32 -fda spark.img

install :
	$(MAKE) img
	$(IMGTOL) w a: spark.img

clean :
	-$(DEL) *.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb
	-$(DEL) *.sys

src_only :
	$(MAKE) clean
	-$(DEL) spark.img
