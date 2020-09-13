package=libgmp
$(package)_version=6.1.1
$(package)_download_path=https://gmplib.org/download/gmp/
$(package)_file_name=gmp-$($(package)_version).tar.bz2
$(package)_sha256_hash=a8109865f2893f1373b0a8ed5ff7429de8db696fc451b1036bd7bdf95bbeffd6
$(package)_dependencies=
$(package)_config_opts=--enable-cxx --disable-shared

ifeq ($(build_os),mingw32)
  CPPFLAGS=-D__USE_MINGW_ANSI_STDIO
endif

define $(package)_config_cmds
  CC_FOR_BUILD=x86_64-linux-gnu-gcc \
  $($(package)_autoconf) --host=$(host) --build=$(build)
endef

define $(package)_build_cmds
  $(MAKE) CPPFLAGS='-fPIC'
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install ; echo '=== staging find for $(package):' ; find $($(package)_staging_dir)
endef
