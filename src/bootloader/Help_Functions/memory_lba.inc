%include "../Help_Functions/struct.inc"

;-----------------------------------------
;	test_bios_extension
;	Called with
;	DL = drive number
;-----------------------------------------
test_bios_extension:
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13
    jc .error
    ret
    .error:
        cli
        hlt

;-----------------------------------------
;	load_sector_lba
;	Called with
;	DL = drive number
;   CX = Count of sectors wished to be loaded
;   EAX = LBA
;   ES:BX = pointer to memory buffer
;-----------------------------------------
load_sector_lba:
    SetMemberWord(cx, DAP, DAP_struct.Count)
    SetMemberDWord(eax, DAP, DAP_struct.LBA_Low)
    SetMemberWord(es, DAP, DAP_struct.Segment)
    SetMemberWord(bx, DAP, DAP_struct.Offset)

    xor ax, ax
    SetMemberByte(al, DAP, DAP_struct.Reserved)

    mov al, 0x10
    SetMemberByte(al, DAP, DAP_struct.Size)

    xor ax, ax
    mov ds, ax
    lea ax, [DAP]
    mov si, ax

    mov ah, 0x42
    int 0x13

    ret