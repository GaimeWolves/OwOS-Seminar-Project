# gdb -ex "file build/os/kernel.elf" -ex "set architecture i386:x86-64:intel" -ex "target remote localhost:1234"

qemu-system-i386 -s -S -no-reboot -no-shutdown -chardev stdio,id=char0,mux=on,logfile=log.txt,signal=off -serial chardev:char0 -hda build/disk.iso -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0 & sleep 1 && gdb -ex "target remote localhost:1234" -ex "symbol-file build/os/kernel.elf"