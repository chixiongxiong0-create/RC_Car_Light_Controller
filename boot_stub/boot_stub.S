.syntax unified
.cpu cortex-m7
.fpu fpv5-d16
.thumb

.equ APP_VECTOR_TABLE, 0x08020000
.equ SCB_VTOR,         0xE000ED08

.section .isr_vector,"a",%progbits
.global BootStub_VectorTable
.type BootStub_VectorTable, %object
BootStub_VectorTable:
    .word _boot_stack_top
    .word BootStub_Reset_Handler
    .word BootStub_Trap
    .word BootStub_Trap
    .word BootStub_Trap
    .word BootStub_Trap
    .word BootStub_Trap
    .word 0
    .word 0
    .word 0
    .word 0
    .word BootStub_Trap
    .word BootStub_Trap
    .word 0
    .word BootStub_Trap
    .word BootStub_Trap
.size BootStub_VectorTable, .-BootStub_VectorTable

.section .text.BootStub_Reset_Handler,"ax",%progbits
.align 2
.global BootStub_Reset_Handler
.type BootStub_Reset_Handler, %function
.thumb_func
BootStub_Reset_Handler:
    ldr r0, =APP_VECTOR_TABLE
    ldr r1, [r0]
    ldr r2, [r0, #4]
    ldr r3, =SCB_VTOR
    str r0, [r3]
    dsb
    isb
    msr msp, r1
    bx r2
.size BootStub_Reset_Handler, .-BootStub_Reset_Handler

.section .text.BootStub_Trap,"ax",%progbits
.align 2
.global BootStub_Trap
.type BootStub_Trap, %function
.thumb_func
BootStub_Trap:
    b BootStub_Trap
.size BootStub_Trap, .-BootStub_Trap
