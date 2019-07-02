include $(TOPDIR)/rules.mk

PKG_NAME:=basicstation
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)    
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=$(PKG_NAME)
	MAINTAINER:=skerlan <myEmail.add>
endef

define Package/$(PKG_NAME)/description
	[Basic Station](https://doc.sm.tc/station) is a LoRaWAN Gateway implementation, including features like
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/lgw
	$(CP) $(PKG_BUILD_DIR)/build-linux-std/include/lgw/* $(1)/usr/include/lgw
	$(INSTALL_DIR) $(1)/usr/include/mbedtls
	$(CP) $(PKG_BUILD_DIR)/build-linux-std/include/mbedtls/* $(1)/usr/include/mbedtls
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/build-linux-std/lib/*.a $(1)/usr/lib/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/build-linux-std/bin/station $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/station
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/examples/live-s2.sm.tc/* $(1)/etc/station
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
