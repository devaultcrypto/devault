package=rocksdb
$(package)_version=5.18.3
$(package)_download_path=https://github.com/facebook/rocksdb/archive/
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=7fb6738263d3f2b360d7468cf2ebe333f3109f3ba1ff80115abd145d75287254
$(package)_build_opts= CC="$($(package)_cc)"
$(package)_build_opts= CXX="$($(package)_cxx)"

ifeq ($(host_os),mingw32)
	sys_os=Windows
else
ifeq ($(host_os),darwin)
	sys_os=Darwin
else
ifeq ($(host_os),linux)
	sys_os=Linux
else  
	sys_os=$(host_os)
endif	
endif	
endif	


define $(package)_preprocess_cmds
endef

define $(package)_set_vars
endef

define $(package)_config_cmds
endef

define $(package)_build_cmds
	CC="$($(package)_cc)" CXX="$($(package)_cxx)" cmake . -DCMAKE_SYSTEM_NAME=$(sys_os) -DWITH_TESTS=0 -DWITH_GFLAGS=0 -DPORTABLE=1 -DWITH_RUNTIME_DEBUG=0 -DWITH_TOOLS=0
endef

define $(package)_stage_cmds
	$(MAKE) rocksdb
	mkdir $($(package)_staging_prefix_dir)/lib
	cp $($(package)_build_dir)/librocksdb.a $($(package)_staging_prefix_dir)/lib/librocksdb.a
	cp -r $($(package)_build_dir)/include $($(package)_staging_prefix_dir)
endef

define $(package)_postprocess_cmds
endef
