# Evmongoose([中文](https://github.com/zhaojh329/evmongoose/blob/master/README_ZH.md))

![](https://img.shields.io/badge/license-GPLV3-brightgreen.svg?style=plastic "License")

Evmongoose is an asynchronous, event(libev) based multi-protocol embedded networking library 
with functions including TCP, HTTP, WebSocket, MQTT and much more. It is based on [mongoose](https://github.com/cesanta/mongoose)
and [libev](https://github.com/kindy/libev) implementation and it's support Lua API.

Evmongoose supports highly customized to extend your application. Before I started this project, I had never found a HTTP server
library that was satisfied with the event based framework, and those HTTP server libraries could only loop it'sown objects and 
could not add my own objects. For example, I want to monitor a signal or a file through the event.

# Features
* New from evmongoose
    - Using libev programming 
	- Highly customized to extend your application based on libev
	- Lua API(Depends on lua-ev)

* Inherited from mongoose
	- plain TCP, plain UDP, SSL/TLS (over TCP, one-way or two-way)
	- Alternative openssl and mbedtls
	- HTTP client, HTTP server
	- Proxy
	- WebSocket client, WebSocket server
	- MQTT client, MQTT broker
	- CoAP client, CoAP server
	- DNS client, DNS server, async DNS resolver
	- Url Rewrite

# [Example](https://github.com/zhaojh329/evmongoose/blob/master/example)
* [simplest web](https://github.com/zhaojh329/evmongoose/blob/master/example/simplest_web.c)
* [simplest web on ssl](https://github.com/zhaojh329/evmongoose/blob/master/example/simplest_web_ssl.c)
* [http client](https://github.com/zhaojh329/evmongoose/blob/master/example/http_client.c)
* [async DNS resolver](https://github.com/zhaojh329/evmongoose/blob/master/example/async_dns_resolver.c)
* [simplest web on Lua](https://github.com/zhaojh329/evmongoose/blob/master/example/simplest_web.lua)

# How To Compile
## For Ubuntu
### Install dependency Libraries
* libev-dev libssl-dev lua5.1 liblua5.1-0-dev

		sudo apt install libev-dev libssl-dev lua5.1 liblua5.1-0-dev

* lua-ev

		git clone https://github.com/brimworks/lua-ev.git
		cd lua-ev
		cmake . -DINSTALL_CMOD=$(lua -e "for k in string.gmatch(package.cpath .. \";\", \"([^;]+)/..so;\") do if k:sub(1,1) == \"/\" then print(k) break end end")
		make && sudo make install
    
### Install Evmongoose(HTTPS Support Default)
    git clone https://github.com/zhaojh329/evmongoose.git
    cd evmongoose
    mkdir build
    cd build
    cmake ..
    make && sudo make install

### Install Evmongoose(Disable HTTPS Support)
    git clone https://github.com/zhaojh329/evmongoose.git
    cd evmongoose
    mkdir build
    cd build
    cmake .. -DHTTPS_SUPPORT=OFF
    make && sudo make install

## For OpenWRT/LEDE
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
	
# C API reference manual
Evmongoose dose not change the usage of API in mongoose and libev, 
so please refer to the API Manual of [mongoose](https://docs.cesanta.com/mongoose/master) and libev.
Only one thing to notice is that mg_mgr_poll is no longer invoked when using evmongoose.

In addition, evmongoose added a new API: mg_mgr_set_loop, which is used to set libev's loop for Mgr.
If the function is not called, the Mgr will use the default loop:EV_DEFAULT.

# [Lua API reference manual](https://github.com/zhaojh329/evmongoose/wiki/Lua-API-reference-manual)

# How To Contribute
Feel free to create issues or pull-requests if you have any problems.

**Please read [contributing.md](https://github.com/zhaojh329/evmongoose/blob/master/contributing.md)
before pushing any changes.**

# Thanks for the following project
* [mongoose](https://github.com/cesanta/mongoose)
* [libev](https://github.com/kindy/libev)
* [lua-ev](https://github.com/brimworks/lua-ev)
* [lua-mongoose](https://github.com/shuax/lua-mongoose)

# If the project is helpful to you, please do not hesitate to star. Thank you!
