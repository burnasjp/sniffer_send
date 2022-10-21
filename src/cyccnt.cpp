#include <cstddef>
#include <cstdint>

#include "cyccnt.h"

//volatile unsigned int *CycCnt::DWT_CYCCNT; //address of the register
unsigned int CycCnt::SCLK;
unsigned int CycCnt::SCLK_1us;
