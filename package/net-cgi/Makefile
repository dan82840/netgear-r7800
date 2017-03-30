# 
# Copyright (C) 2009 OpenWrt.org
#

include $(TOPDIR)/rules.mk

PKG_NAME:=net-cgi
PKG_RELEASE:=1
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

WEB_PATH:=www

include $(INCLUDE_DIR)/package.mk

PKG_GIT_TREEISH:=net-cgi-dni227

define Package/net-cgi
  SECTION:=utils
  CATEGORY:=Base system
  DEFAULT:=y
  TITLE:=The mini_httpd CGI interface
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/net-cgi.git/* $(PKG_BUILD_DIR)/
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/$(WEB_PATH)/cgi-bin
	chmod a+x $(PKG_BUILD_DIR)/sh/*
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR) && \
		./configure \
	)
endef

define Build/Compile
	(cd $(PKG_BUILD_DIR) && \
		make www \
	)
endef

define Package/net-cgi/install
	$(MAKE) -C $(PKG_BUILD_DIR) install INS_DIR_PRE=$(1)

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/support_soap_list $(TOPDIR)/bin/

	$(LN) /tmp/openvpn/windows.zip $(1)/$(WEB_PATH)/windows.zip
	$(LN) /tmp/openvpn/nonwindows.zip $(1)/$(WEB_PATH)/nonwindows.zip
	$(LN) /tmp/openvpn/smart_phone.ovpn $(1)/$(WEB_PATH)/smart_phone.ovpn
endef

$(eval $(call BuildPackage,net-cgi))
