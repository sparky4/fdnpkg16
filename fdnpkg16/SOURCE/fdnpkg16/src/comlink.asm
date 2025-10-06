;
; Template program for chaining into another program. This code is not used
; as-is, it's a reference used in FDNPKG for computing COM "link" files.
;
; Copyright (C) 2017-2018 Mateusz Viste
; Changes by TK Chia
;
; To be compiled with NASM: NASM -f bin test.asm -o test.com
;
; Then, append the full path and filename of the program to chain to, followed
; by a NULL terminator (0).
;

org 100h

; by default the COM file gets all available memory, so I need to free some
; of it so there is enough available for the child program.
start:
mov sp, 0          ; these should be the program's first two instructions;
mov bx, 1000h      ; pkginst.c will fill in these operands with better
                   ; values that depend on the program path length

mov ah, 4Ah        ; resize memory block at PSP (ES already set to it)
int 21h

; now set up the pointer to command line arguments in the parameters block
; (offset is pre-set already, but segment still needs to be set)
mov bx, params     ; parameter block (at ES:BX)
mov [bx+cmd_seg-params], ds
mov [bx+fcb1_seg-params], ds
mov [bx+fcb2_seg-params], ds

; execute the child program now
mov ax, 4B00h      ; load and execute program
mov dx, PROGNAME   ; ASCIIZ path of program
int 21h

; if the child could not even be run, return an exit code of 127
mov al, 127
jc exit

; otherwise, get the child's exit code, to propagate up to our parent
mov ah, 4Dh
int 21h

; exit (this) program
exit:
mov ah, 4Ch
int 21h

align 2

params:
environ dw 0       ; segment of environment, 0 means to use parent's one
cmd_off dw 80h     ; ptr to command line's offset
cmd_seg dw 0       ; ptr to command line's segment
fcb1_off dw 5Ch    ; pointers to file control blocks
fcb1_seg dw 0
fcb2_off dw 6Ch
fcb2_seg dw 0

PROGNAME:
; nothing here now - once compiled, a program's full path should be appended
; here, followed by a single 0 byte.
