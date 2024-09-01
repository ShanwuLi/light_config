# light_config

#### 介绍
轻量级配置工具

#### 软件架构
软件架构说明


#### 安装教程

1.  安装gcc编译器
2.  在source目录下执行make
3.  编译完成后在test目录下生成light_config可执行文件

#### 使用说明

1.  在test目录下使用./light_config --help
2.  按照help指引输入指定参数，也可采用默认参数
3.  查看解析后的配置文件

#### 配置文件语法
##### menu config语法
menu config的语法主要包含3部分，分别为
1. 宏名称
2. 宏赋值符
3. 宏值

定义的宏即为：宏名称 宏赋值符 宏值

示例如下：
	CONFIG_TEST_MENU = y

	CONFIG_TEST_MENU_BAUD_RATE = 115200

###### 宏名称
宏名称可以是任意数字，字母或下划线组成的标识符，且不能以数字开头。

###### 宏赋值符
宏赋值符包括4种，分别为: =, :=, ?=, +=

=类型的宏会输出到头文件中，其他类型的则输出到makefile文件中。

###### 宏值
宏值定义为字符串，且可以使用[]来选择是否引用default config中的宏值，没有使用[]定义的宏值将不会
索引default config中的宏值，可以通过[*]来直接获取default config中的宏值，每个宏值后面可以跟
一个逻辑运算表达式，并将其用于依赖构建。

##### default config语法
default config用于选择或使能menu config中的宏，从而完成配置文件的依赖构建。

#### 配置文件实例
```makefile
# "menu config"
OS_TOPDIR                     = .
CONFIG_MENU                   = y
DEFALUT                       = y
CONFIG_UART                   = ["LOGO"] & <CONFIG_MENU>
UART_EN                       = [] & (<CONFIG_MENU> & <CONFIG_UART>)
CONFIG_BAUD_RATE              = [@menu([9600], [11520], [115200])] & <UART_EN>
CONFIG_BAUD_STOP_BIT          = [@menu([1], [1.5], [2])]           & <UART_EN>
CONFIG_UART_NAME              = [*]                                & <UART_EN>
CONFIG_UART_OTHER-<DEFALUT>   = [*]                                & <UART_EN>
CONFIG_TEST_NAME              = [@<DEFALUT> ? (["default_name"], ["other_name"])] & <UART_EN>

SUB_DIR                       = <OS_TOPDIR>/subdir
-include                      "<SUB_DIR>/subcfg.lc"

CSRCS-<UART_EN> += <OS_TOPDIR>/uart.c
ASMCSRCS-<UART_EN> += <OS_TOPDIR>/uart_asm.S
CXXSRC += <OS_TOPDIR>/menu.cpp

```


```makefile
# "default config"
CONFIG_UART                   = "INSTAED_LOGO"
UART_EN                       = y
CONFIG_BAUD_RATE              = 115200
CONFIG_BAUD_STOP_BIT          = 2
CONFIG_UART_NAME              = "stm32f103 uart1"
CONFIG_UART_OTHER             = "other_name"

```




#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
