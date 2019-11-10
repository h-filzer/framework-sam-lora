/*
   
   Microchip Atmel SAMR3x Flash Utility ver 1.00 PlatformIO
  
     Copyright (C) 2019 Georgi Angelov. All rights reserved.
     Author: Georgi Angelov <the.wizarda@gmail.com> WizIO
         http://www.wizio.eu/
         https://github.com/Wiz-IO/platform-sam-lora
  
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
  
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
   3. Neither the name WizIO nor the names of its contributors may be
      used to endorse or promote products derived from this software
      without specific prior written permission.
  
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
   OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
  
*/

#include <stdint.h>
#include <samr34.h>
#include "cfg.h"

#define BOOT_DELAY 0xFFFFF

void UART_INIT(void);
int available(void);
void putChar(char c);
uint8_t getChar(void);
void putString(char *str);
void boot_task(void);

void delayUs(uint32_t delay)
{
    for (uint32_t i = 0; i < delay; i++)
        __asm__ __volatile__("");
}

int main(void)
{
    UART_INIT();
    //putString("ATSAM34\n");
    uint32_t cnt = BOOT_DELAY;
    while (1)
    {
        if (available())
        {
            if (ACK == getChar())
                boot_task();
        }

        if (0 == cnt--)
        {
            cnt = BOOT_DELAY;
            //check_application();
            //putChar('*');
        }
    }
}