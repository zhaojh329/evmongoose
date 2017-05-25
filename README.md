# Evmongoose
**[中文介绍](https://github.com/zhaojh329/evmongoose/blob/master/README_ZH.md)**

![](https://img.shields.io/badge/license-GPLV3-brightgreen.svg?style=plastic "License")

Evmongoose is an api friendly and scalable HTTP server library based on mongoose and libev. Evmongoose supports
highly customized to extend your application. Before I started this project, I had never found a HTTP server
library that was satisfied with the event based framework, and those HTTP server libraries could only loop it's
own objects and could not add my own objects. For example, I want to monitor a signal or a file through the event.

# Features
* Supports Highly customized to extend your application
* Supports https
* The SSL library can select OpenSSL and mbedtls

## **[Example](https://github.com/zhaojh329/evmongoose/blob/master/example.c)**

## How To Compile
    git clone https://github.com/zhaojh329/evmongoose.git
    cd evmongoose
    mkdir build
    cd build
    cmake ..
    make