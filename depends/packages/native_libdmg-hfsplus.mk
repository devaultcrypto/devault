package=native_libdmg-hfsplus
$(package)_version=3e5fd3fb56bc9ff03beb535979e33dcf83fe1f70
$(package)_download_path=https://github.com/fanquake/libdmg-hfsplus/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=6ad6116755d1442feeda98cfed818153d1f4433627ce6576347a78432eb0b228
$(package)_build_subdir=build

define $(package)_preprocess_cmds
  mkdir build
endef

define $(package)_config_cmds
  cmake -DCMAKE_INSTALL_PREFIX:PATH=$(build_prefix)/bin ..
endef

define $(package)_build_cmds
  $(MAKE) -C dmg
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) -C dmg install
endef
