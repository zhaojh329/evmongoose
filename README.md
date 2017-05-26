# Evmongoose([中文](https://github.com/zhaojh329/evmongoose/blob/master/README_ZH.md))

![](https://img.shields.io/badge/license-GPLV3-brightgreen.svg?style=plastic "License")

Evmongoose is an api friendly and scalable HTTP server library based on mongoose and libev. Evmongoose supports
highly customized to extend your application. Before I started this project, I had never found a HTTP server
library that was satisfied with the event based framework, and those HTTP server libraries could only loop it's
own objects and could not add my own objects. For example, I want to monitor a signal or a file through the event.

# Features
* New from evmongoose
	- Using libev programming
	- Highly customized to extend your application based on libev
	- Lua(In development)

* Inherited from mongoose
	- HTTP client, HTTP server
	- Alternative openssl and mbedtls
	- DNS client, DNS server, async DNS resolver
	- WebSocket client, WebSocket server
	- ...

# [Example](https://github.com/zhaojh329/evmongoose/blob/master/example/simplest_web.c)

# How To Compile
## For Ubuntu
### Install dependency Libraries
    sudo apt install libev-dev libssl-dev
    
### Install Evmongoose(HTTPS Support Default)
    git clone https://github.com/zhaojh329/evmongoose.git
    cd evmongoose
    mkdir build
    cd build
    cmake ..
    make

### Install Evmongoose(Disable HTTPS Support)
    git clone https://github.com/zhaojh329/evmongoose.git
    cd evmongoose
    mkdir build
    cd build
    cmake .. -DHTTPS_SUPPORT=OFF
    make

## For OpenWRT/LEDE
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
	
# API reference manual
Evmongoose dose not change the usage of API in mongoose and libev, 
so please refer to the API Manual of [mongoose](https://docs.cesanta.com/mongoose/master) and libev.
Only one thing to notice is that mg_mgr_poll is no longer invoked when using evmongoose.
    
# Thanks for the following project
* [mongoose](https://github.com/cesanta/mongoose)
* [libev](https://github.com/kindy/libev)
