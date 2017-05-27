# Evmongoose

![](https://img.shields.io/badge/license-GPLV3-brightgreen.svg?style=plastic "License")

Evmongoose是一个API接口友好和可伸缩的HTTP服务器库，它基于mongoose和libev实现。Evmongoose支持高度的可定制化
来扩展你的应用程序。在开始这个项目之前，我一直都没有找到一个令我满意的基于事件框架的HTTP服务器库。那些HTTP
服务器库只能loop它自己的对象，不能添加我自己的对象。比如我想基于事件框架监视某个信号（比如SIGINT）或者某个文件。

# 特性
* 新特性
    - 使用libev编程
	- 高度的可定制化
	- Lua（开发中）

* 继承自mongoose
	- HTTP客户端, HTTP服务器
	- SSL库可选：OpenSSL和mbedtls，对于存储苛刻的系统可选择mbedtls
	- DNS客户端，DNS服务器，异步DNS解析
	- WebSocket客户端，WebSocket服务器
	- Url重写
	- ...

## 示例程序
* [simplest web](https://github.com/zhaojh329/evmongoose/blob/master/example/simplest_web.c)
* [simplest web on ssl](https://github.com/zhaojh329/evmongoose/blob/master/example/simplest_web_ssl.c)
* [http client](https://github.com/zhaojh329/evmongoose/blob/master/example/http_client.c)
* [async DNS resolver](https://github.com/zhaojh329/evmongoose/blob/master/example/async_dns_resolver.c)

# 编译
## 在Ubuntu上运行
### 安装依赖库
    sudo apt install libev-dev libssl-dev
    
### 安装evmongoose（默认支持HTTPS）
    git clone https://github.com/zhaojh329/evmongoose.git
    cd evmongoose
    mkdir build
    cd build
    cmake ..
    make

### 安装evmongoose（禁止HTTPS）
    git clone https://github.com/zhaojh329/evmongoose.git
    cd evmongoose
    mkdir build
    cd build
    cmake .. -DHTTPS_SUPPORT=OFF
    make

## OpenWRT/LEDE
	git clone https://github.com/zhaojh329/evmongoose.git
	cp evmongoose/openwrt/ openwrt_dir/package/evmongoose -r
	cd openwrt_dir
	make menuconfig
	Libraries  --->
	    Networking  --->
	        <*> evmongoose
	            Configuration  --->
	                SSl (mbedtls)  --->
	
	make package/evmongoose/compile V=s
	
# API参考手册
Evmongoose并没有改变mongoose和libev的API用法，所以请参考
[mongoose](https://docs.cesanta.com/mongoose/master)
和libev的API参考手册。只有一点需要注意，使用evmongoose时不再调用mg_mgr_poll。

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

# 技术交流
QQ群：153530783

# 如果该项目对您有帮助，请随手star，谢谢！
