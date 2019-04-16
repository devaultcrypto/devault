package=gflags
$(package)_version=2.2.2
$(package)_download_path=https://github.com/gflags/gflags/archive/
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf

define $(package)_preprocess_cmds
$(package)_config_opts_linux=--with-pic
endef

define $(package)_set_vars
endef

define $(package)_config_cmds
  cmake . -DCMAKE_INSTALL_PREFIX=$($(package)_staging_prefix_dir)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) INSTALL_PATH=$($(package)_staging_prefix_dir) install
endef

define $(package)_postprocess_cmds
endef
