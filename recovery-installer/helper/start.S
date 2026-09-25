.text
.global _start
.type _start, %function
_start:
    mov x0, sp
    bl c_start
    mov x8, #93
    svc #0
.size _start, .-_start
