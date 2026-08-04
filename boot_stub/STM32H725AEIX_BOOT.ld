ENTRY(BootStub_Reset_Handler)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 128K
}

_boot_stack_top = 0x24050000;

SECTIONS
{
    .isr_vector ORIGIN(FLASH) :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    .boot_reset 0x08000040 :
    {
        KEEP(*(.text.BootStub_Reset_Handler))
    } > FLASH

    .boot_trap :
    {
        KEEP(*(.text.BootStub_Trap))
    } > FLASH

    .ARM.attributes 0 :
    {
        KEEP(*(.ARM.attributes))
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}

ASSERT(SIZEOF(.isr_vector) == 0x40,
       "Boot-stub core vector table must remain exactly 16 words")
ASSERT(BootStub_Reset_Handler == 0x08000040,
       "Boot-stub reset entry must remain fixed at 0x08000040")
ASSERT((LOADADDR(.boot_trap) + SIZEOF(.boot_trap)) <=
       (ORIGIN(FLASH) + LENGTH(FLASH)),
       "Boot stub must fit entirely in flash sector 0")
