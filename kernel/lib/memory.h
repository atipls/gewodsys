#pragma once

#include <stdint.h>

void RtZeroMemory(void *address, uint64_t size);
void RtCopyMemory(void *destination, const void *source, uint64_t size);
void RtFillMemory(void *address, uint64_t size, uint8_t value);