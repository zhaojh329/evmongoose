# Evmongoose([github](https://github.com/zhaojh329/evmongoose))

![](https://img.shields.io/badge/license-GPLV3-brightgreen.svg?style=plastic "License")

![](https://github.com/zhaojh329/image/blob/master/evmongoose.png)

Evmongoose是一个异步的、基于事件框架(libev)的集成多种协议的嵌入式网络库，包括：TCP、HTTP、WebSocket、MQTT等等。
它基于[mongoose](https://github.com/cesanta/mongoose)和[libev](https://github.com/kindy/libev)实现，并且支持Lua API。

Evmongoose支持高度的可定制化来扩展你的应用程序。在开始这个项目之前，我一直都没有找到一个令我满意的基于事件框架的HTTP服务器库。那些HTTP
服务器库只能loop它自己的对象，不能添加我自己的对象。比如我想基于事件框架监视某个信号（比如SIGINT）或者某个文件。

# 特性
* 新特性
    - 使用libev编程
	- 高度的可定制化
	- Lua API（除了对evmongoose的Lua封装，还包括常用的posix C API）

* 继承自mongoose
	- TCP服务器/TCP客户端、UDP服务器/UDP客户端, SSL/TLS
	- SSL库可选择OpenSSL或者mbedtls，对于存储苛刻的系统可选择mbedtls
	- HTTP客户端，HTTP服务器
	- HTTP文件上传
	- HTTP代理
	- WebSocket客户端，WebSocket服务器
	- MQTT客户端，MQTT代理
	- CoAP客户端，CoAP服务器
	- DNS客户端，DNS服务器，异步DNS解析
	- Url重写

# [示例程序](https://github.com/zhaojh329/evmongoose/blob/master/example)
* [simplest web](https://github.com/zhaojh329/evmongoose/blob/master/example/simplest_web.c)
* [simplest web on ssl](https://github.com/zhaojh329/evmongoose/blob/master/example/simplest_web_ssl.c)
* [http client](https://github.com/zhaojh329/evmongoose/blob/master/example/http_client.c)
* [async DNS resolver](https://github.com/zhaojh329/evmongoose/blob/master/example/async_dns_resolver.c)
* [Lua](https://github.com/zhaojh329/evmongoose/blob/master/example/lua)

# 编译
## 在Ubuntu上运行
### 安装依赖库
* libev-dev libssl-dev lua5.1 liblua5.1-0-dev lua-zlib
	
		sudo apt install libev-dev libssl-dev lua5.1 liblua5.1-0-dev lua-zlib

* lua-ev

		git clone https://github.com/brimworks/lua-ev.git
		cd lua-ev
		cmake . -DINSTALL_CMOD=$(lua -e "for k in string.gmatch(package.cpath .. \";\", \"([^;]+)/..so;\") do if k:sub(1,1) == \"/\" then print(k) break end end")
		make && sudo make install
    
### 安装evmongoose（默认支持HTTPS）
    git clone https://github.com/zhaojh329/evmongoose.git
    cd evmongoose
    mkdir build
    cd build
    cmake ../
    make && sudo make install

### 安装evmongoose（禁止HTTPS）
    git clone https://github.com/zhaojh329/evmongoose.git
    cd evmongoose
    mkdir build
    cd build
    cmake ../ -DHTTPS_SUPPORT=OFF
    make && sudo make install

## OpenWRT/LEDE
	git clone https://github.com/zhaojh329/lua-ev-openwrt.git
	cp -r lua-ev-openwrt openwrt_dir/package/lua-ev
	
	git clone https://github.com/zhaojh329/evmongoose.git
	cp -r evmongoose/openwrt openwrt_dir/package/evmongoose
	
	cd openwrt_dir
	./scripts/feeds update -a
	./scripts/feeds install -a
	
	make menuconfig
	Libraries  --->
	    Networking  --->
	        <*> evmongoose
	            Configuration  --->
	                SSl (mbedtls)  --->
	
	make package/evmongoose/compile V=s

## 通用嵌入式环境
### [首先交叉编译Lua](https://github.com/zhaojh329/lua-5.1.5-mod)

### 交叉编译lib-ev
	git clone https://github.com/kindy/libev.git
	cd libev/src
	sh ./autogen.sh
	
	# 配置交叉编译器执行环境
	export PATH=/home/zjh/lede/staging_dir/toolchain-mipsel_24kc_gcc-5.4.0_musl/bin:$PATH
	./configure --host=mipsel-openwrt-linux --prefix=`pwd`/obj
	make && make install
	
### 交叉编译Lua-ev
	git clone https://github.com/brimworks/lua-ev.git
	cd lua-ev
	
	# 配置交叉编译器执行环境
	export PATH=/home/zjh/lede/staging_dir/toolchain-mipsel_24kc_gcc-5.4.0_musl/bin:$PATH
	
	# 其中xxxxx代码你的交叉编译器的跟目录
	cmake . -DCMAKE_C_COMPILER=mipsel-openwrt-linux-gcc -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY -DCMAKE_FIND_ROOT_PATH=xxxxx
	make
	
### 交叉编译evmongoose
	git clone https://github.com/zhaojh329/evmongoose.git
    cd evmongoose
	
	# 配置交叉编译器执行环境
	export PATH=/home/zjh/lede/staging_dir/toolchain-mipsel_24kc_gcc-5.4.0_musl/bin:$PATH
	
	# 其中xxxxx代码你的交叉编译器的跟目录
	cmake . -DCMAKE_C_COMPILER=mipsel-openwrt-linux-gcc -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY -DCMAKE_FIND_ROOT_PATH=xxxxx
	make
	
# C API参考手册
Evmongoose并没有改变mongoose和libev的API用法，所以请参考
[mongoose](https://docs.cesanta.com/mongoose/master)
和libev的API参考手册。只有一点需要注意，使用evmongoose时不再调用mg_mgr_poll。

另外，evmongoose新增了一个API：mg_mgr_set_loop，用于给mgr设置libev的loop，如果没有显示调用
该函数，mgr将使用默认loop：EV_DEFAULT。

# [Lua API参考手册](https://github.com/zhaojh329/evmongoose/wiki/Lua-API-reference-manual)

# 贡献代码

Evmongoose使用github托管其源代码，贡献代码使用github的PR(Pull Request)的流程，十分的强大与便利:

1. [创建 Issue](https://github.com/zhaojh329/evmongoose/issues/new) - 对于较大的
	改动(如新功能，大型重构等)最好先开issue讨论一下，较小的improvement(如文档改进，bugfix等)直接发PR即可
2. Fork [evmongoose](https://github.com/zhaojh329/evmongoose) - 点击右上角**Fork**按钮
3. Clone你自己的fork: ```git clone https://github.com/$userid/evmongoose.git```
4. 创建dev分支，在**dev**修改并将修改push到你的fork上
5. 创建从你的fork的**dev**分支到主项目的**dev**分支的[Pull Request] -  
	[在此](https://github.com/zhaojh329/evmongoose)点击**Compare & pull request**
6. 等待review, 需要继续改进，或者被Merge!
	
## 感谢以下项目提供帮助
* [mongoose](https://github.com/cesanta/mongoose)
* [libev](https://github.com/kindy/libev)
* [lua-ev](https://github.com/brimworks/lua-ev)
* [lua-mongoose](https://github.com/shuax/lua-mongoose)

# 技术交流
QQ群：153530783

# 如果该项目对您有帮助，请随手star，谢谢！
