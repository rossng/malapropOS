init-disk :
	@dd of=disk.bin if=/dev/zero count=16 bs=65536

rm-disk :
	@rm -f disk.bin

launch-disk :
	@python disk.py --host=127.0.0.1 --port=1235 --file=disk.bin --block-num=65536 --block-len=16

print-%  : ; @echo $* = $($*)
