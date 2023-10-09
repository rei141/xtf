include $(ROOT)/build/common.mk

NAME      := necofuzz-vmx
CATEGORY  := utility
TEST-ENVS := hvm64

TEST-EXTRA-CFG := extra.cfg.in

obj-perenv += main.o binc.o fuzz.o msr.o pci.o vmx.o

include $(ROOT)/build/gen.mk
