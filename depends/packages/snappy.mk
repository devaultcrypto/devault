package=snappy
$(package)_version=1.1.7
$(package)_download_path=https://github.com/google/snappy/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=3dfa02e873ff51a11ee02b9ca391807f0c8ea0529a4924afa645fbf97163f9d4

define $(package)_preprocess_cmds
endef

define $(package)_set_vars
endef

define $(package)_config_cmds
  cmake . -DCMAKE_INSTALL_PREFIX=$($(package)_staging_prefix_dir) -DBUILD_SHARED_LIBS=0 -DSNAPPY_BUILD_TESTS=0
endef

 define $(package)_build_cmds
  $(MAKE)
endef

 define $(package)_stage_cmds
  $(MAKE) install
endef

define $(package)_postprocess_cmds
endef
