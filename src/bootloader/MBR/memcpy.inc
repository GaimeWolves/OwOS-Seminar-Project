%define SRC_SEG 0x7c0
%define DEST_SEG 0x070

[bits 16]

;run 512 bytes, 256 words
mov cx, 256 

;Load source address to ds:si
mov ax, SRC_SEG
mov ds, ax
xor si, si

;Load destination address to es:di
mov ax, DEST_SEG
mov es, ax
xor di, di

;Copy 2*cx bytes from ds:si to es:di
rep movsw