# Evmongoose

![](https://img.shields.io/badge/license-GPLV3-brightgreen.svg?style=plastic "License")

Evmongoose是一个API接口友好和可伸缩的HTTP服务器库，它基于mongoose和libev实现。Evmongoose支持高度的可定制化
来扩展你的应用程序。在开始这个项目之前，我一直都没有找到一个令我满意的基于事件框架的HTTP服务器库。那些HTTP
服务器只能loop它自己的对象，不能添加我自己的对象。比如我想基于事件框架监视某个信号（比如SIGINT）或者某个文件。

# 特性
* 高度的可定制化
* 支持HTTP和HTTPS
* SSL库可选：OpenSSL和mbedtls，对于存储苛刻的系统可选择mbedtls