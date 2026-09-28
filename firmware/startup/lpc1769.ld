/* lpc1769.ld - 512 Ko de flash, 32 Ko de SRAM locale */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x10000000, LENGTH = 32K
}
_estack = ORIGIN(RAM) + LENGTH(RAM) - 32;   /* 32 o réservés par la ROM IAP */
ENTRY(Reset_Handler)
SECTIONS
{
  .text : {
    KEEP(*(.isr_vector))
    *(.text*) *(.rodata*)
    . = ALIGN(4);
  } > FLASH
  _sidata = LOADADDR(.data);
  .data : { _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT > FLASH
  .bss (NOLOAD) : { _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > RAM
}
