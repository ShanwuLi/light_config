/*
MIT License
Copyright (c) 2024 light config https://gitee.com/lsw-elecm/light_config

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define LC_TOPDIR F:/light_config/examples
#define OS_TOPDIR .
#define CONFIG_MENU
#define DEFALUT
#define CONFIG_UART "LOGO"
#define UART_EN
#define CONFIG_BAUD_RATE 115200
#define CONFIG_BAUD_STOP_BIT 2
#define CONFIG_UART_NAME "stm32f103 uart1"
#define CONFIG_UART_OTHER "other_name"
#define CONFIG_TEST_NUM "default_name"
#define CONFIG_UART_NUM 25
#define SUB_DIR ./subdir

#endif /* __CONFIG_H__*/
