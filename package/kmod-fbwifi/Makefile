include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-fbwifi
PKG_VERSION:=1
PKG_RELEASE:=

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/kmod-fbwifi

include $(INCLUDE_DIR)/package.mk

define KernelPackage/fbwifi
  SUBMENU:=Other modules
  TITLE:=device module
  VERSION:=$(LINUX_VERSION)+$(PKG_VERSION)-$(BOARD)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/fbwifi_device.$(LINUX_KMOD_SUFFIX) \
	$(PKG_BUILD_DIR)/fbwifi_url_dev.$(LINUX_KMOD_SUFFIX) \
	$(PKG_BUILD_DIR)/ipt_httphijack.$(LINUX_KMOD_SUFFIX) \
	$(PKG_BUILD_DIR)/ipt_redirect.$(LINUX_KMOD_SUFFIX) \
#  AUTOLOAD:=$(call AutoLoad,97,fbwifi)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Build/Compile
endef

#define KernelPackage/kcode/install
#	$(INSTALL_DIR) $(1)/lib/device/
#endef

$(eval $(call KernelPackage,fbwifi))
