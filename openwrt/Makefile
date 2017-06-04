
include $(TOPDIR)/rules.mk

PKG_NAME:=evmongoose
PKG_VERSION:=2017-6-4
PKG_RELEASE:=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/zhaojh329/evmongoose.git
PKG_SOURCE_VERSION:=3ecce94db6caeca7ea778c595928c66f456f25ae
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

PKG_CONFIG_DEPENDS:= CONFIG_EVMONGOOSE_SSL_NONE CONFIG_EVMONGOOSE_SSL_OPENSSL CONFIG_EVMONGOOSE_SSL_MBEDTLS

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/evmongoose
  SECTION:=libs
  CATEGORY:=Libraries
  SUBMENU:=Networking
  TITLE:=An asynchronous, event(libev) based multi-protocol embedded networking library
  URL:=https://github.com/zhaojh329/evmongoose
  DEPENDS:=+libev +lua-ev +EVMONGOOSE_SSL_OPENSSL:libopenssl +EVMONGOOSE_SSL_MBEDTLS:libmbedtls
endef

define Package/evmongoose/description
	Evmongoose is an asynchronous, event(libev) based multi-protocol embedded networking library.
	It is based on mongoose and libev implementation and it's support Lua API.
endef

define Package/evmongoose/config
menu "Configuration"
	depends on PACKAGE_evmongoose

	choice
		prompt "SSl"
		default EVMONGOOSE_SSL_NONE
		
		config EVMONGOOSE_SSL_NONE
			bool "none"
			
		config EVMONGOOSE_SSL_OPENSSL
			bool "openssl"
			
		config MONGOOSE_SSL_MBEDTLS
			bool "mbedtls"
		
	endchoice
endmenu
endef

CMAKE_OPTIONS = -DHTTPS_SUPPORT=OFF

ifeq ($(CONFIG_EVMONGOOSE_SSL_OPENSSL),y)
  CMAKE_OPTIONS = -DHTTPS_SUPPORT=ON
endif

ifeq ($(CONFIG_EVMONGOOSE_SSL_MBEDTLS),y)
  CMAKE_OPTIONS = -DHTTPS_SUPPORT=ON -DUSE_MBEDTLS=ON
endif

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/libevmongoose.so $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/mongoose.h $(1)/usr/include
endef

define Package/evmongoose/install
	$(INSTALL_DIR) $(1)/usr/lib $(1)/usr/lib/lua
	$(CP) $(PKG_BUILD_DIR)/libevmongoose.so $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/lua/evmongoose.so $(1)/usr/lib/lua
endef

$(eval $(call BuildPackage,evmongoose))
