# MIT License
# Copyright (c) 2024 light config  https://gitee.com/lsw-elecm/light_config
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

LC_TOPDIR = F:/light_config/examples
OS_TOPDIR = .
CONFIG_MENU = y
DEFALUT = y
CONFIG_UART = "LOGO"
UART_EN = y
CONFIG_BAUD_RATE = 115200
CONFIG_BAUD_STOP_BIT = 2
CONFIG_UART_NAME = "stm32f103 uart1"
CONFIG_UART_OTHER = "other_name"
CONFIG_TEST_NAME = "default_name"
SUB_DIR = ./subdir
CSRCS += ./uart.c
ASMCSRCS += ./uart_asm.S
CXXSRC += ./menu.cpp
