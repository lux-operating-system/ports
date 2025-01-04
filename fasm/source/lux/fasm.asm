; flat assembler interface for lux x64
; Copyright (c) 1999-2022, Tomasz Grysztar.
;
; Interface by Omar Elghoul, 2025

format ELF64 executable at 0x400000
entry start

include 'modes.inc'

segment readable executable
align 16
start:
    mov esi, _logo
    call display_string

    call get_params
    call init_memory

    mov esi, _memory_prefix
    call display_string

    mov eax, [memory_setting]
    call display_number

    mov esi, _memory_suffix
    call display_string

    call preprocessor
    call parser
    call assembler
    call formatter

    movzx eax, [current_pass]
    inc eax
    call display_number
    mov esi, _passes_suffix
    call display_string
    mov eax, [written_size]
    call display_number
    mov esi, _bytes_suffix
    call display_string

    xor al, al
    call exit_program

; get_params(): parse command line arguments
; params: RDI = argv
; returns: nothing
align 16
get_params:
    mov eax, [rdi+8]
    mov [input_file], eax

    test eax, eax
    jz .usage

    mov eax, [rdi+16]
    mov [output_file], eax

    test eax, eax
    jz .usage

    mov eax, 16384          ; TODO: customize this with the -m option
    mov [memory_setting], eax

    ret

.usage:
    mov esi, _usage
    call display_string

    mov al, 0xFF
    call exit_program

include 'system.inc'

include '..\version.inc'

_copyright db 'Copyright (c) 1999-2022, Tomasz Grysztar',0xA,0

_logo db 'flat assembler  version ',VERSION_STRING,0
_usage db 0xA
       db 'usage: fasm <source> [output]',0xA
       db 'optional settings:',0xA
       db ' -m <limit>         set the limit in kilobytes for the available memory',0xA
       db ' -p <limit>         set the maximum allowed number of passes',0xA
       db ' -d <name>=<value>  define symbolic variable',0xA
       db ' -s <file>          dump symbolic information for debugging',0xA
       db 0
_memory_prefix db '  (',0
_memory_suffix db ' kilobytes memory, x64)',0xA,0
_passes_suffix db ' passes, ',0
_seconds_suffix db ' seconds, ',0
_bytes_suffix db ' bytes.',0xA,0
_no_low_memory db 'failed to allocate memory within 32-bit addressing range',0

include '..\errors.inc'
include '..\symbdump.inc'
include '..\preproce.inc'
include '..\parser.inc'
include '..\exprpars.inc'
include '..\assemble.inc'
include '..\exprcalc.inc'
include '..\x86_64.inc'
include '..\avx.inc'
include '..\formats.inc'

include '..\tables.inc'
include '..\messages.inc'

segment readable writeable

align 4

include '..\variable.inc'

command_line dq ?
memory_setting dd ?
path_pointer dd ?
definitions_pointer dd ?
environment dq ?
timestamp dq ?
start_time dd ?
con_handle dd ?
displayed_count dd ?
last_displayed db ?
character db ?
preprocessing_done db ?

buffer rb 1000h
predefinitions rb 1000h
paths rb 10000h

statbuf rb 96