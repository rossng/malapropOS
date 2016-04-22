init-disk :
	@dd of=disk.bin if=/dev/zero count=16 bs=65536
	@dd of=disk.bin if=bootsector.bin bs=512 count=1 conv=notrunc
	@dd of=disk.bin if=initialfat.bin bs=512 count=16 seek=1 conv=notrunc
	@dd of=disk.bin if=initialroot.bin bs=512 count=32 seek=17 conv=notrunc

rm-disk :
	@rm -f disk.bin

launch-disk :
	@python disk.py --host=127.0.0.1 --port=1235 --file=disk.bin --block-num=8192 --block-len=512

print-%  : ; @echo $* = $($*)
