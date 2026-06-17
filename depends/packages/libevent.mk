package=libevent
$(package)_version=2.1.12-stable
$(package)_download_path=https://github.com/libevent/libevent/releases/download/release-$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=92e6de1be9ec176428fd2367677e61ceffc2ee1cb119035037a27d346b0403bb
# winver_fixup.patch (ported from Bitcoin Core): libevent 2.1.12's evutil.c forces
# _WIN32_WINNT=0x0501 right before <iphlpapi.h>, but Guix's mingw-w64 13 headers use Vista+
# types (PMIB_TCPTABLE2 etc.) unconditionally -> "unknown type" build failure. The patch moves
# the _WIN32_WINNT define to the top so the cppflags_mingw32 below (0x0A00) wins.
$(package)_patches=winver_fixup.patch

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --disable-openssl --disable-libevent-regress --disable-samples
  $(package)_config_opts += --disable-dependency-tracking --enable-option-checking
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_config_opts_linux=--with-pic
  $(package)_cppflags_mingw32=-D_WIN32_WINNT=0x0A00
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/winver_fixup.patch
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) -j$(JOBS)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef
