#include <stdio.h>
#include "color.h"

int main() {

  for (unsigned c = 0; c < 0x10000; c++) {
    RGB565 color16((uint16_t) c);
    uint32_t rgb = color16.rgb();
    RGB565 color16_2(rgb);
    CIELab cie(rgb);

    if (c != color16_2.value || rgb != cie.rgb())
	printf("%04x %04x %08x %08x (%g, %g, %g)\n", c, color16_2.value, rgb, cie.rgb(), cie.L, cie.a, cie.b);
  }

  return 0;
}
