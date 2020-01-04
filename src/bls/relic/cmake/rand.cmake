message(STATUS "Available pseudo-random number generators (default = HASH):")
if (SHOW_MESSAGES)
  
  message("   RAND=HASH      Use the HASH-DRBG generator. (recommended)")
  message("   RAND=UDEV      Use the operating system underlying generator.")
  message("   RAND=FIPS      Use the FIPS 186-2 (CN1) SHA1-based generator.")
  message("   RAND=CALL      Override the generator with a callback.")
  
  message(STATUS "Available random number generator seeders (default = UDEV):")
  
  message("   SEED=WCGR      Use Windows' CryptGenRandom. (recommended)")
  message("   SEED=DEV       Use blocking /dev/random. (recommended)")
  message("   SEED=UDEV      Use non-blocking /dev/urandom. (recommended)")
  message("   SEED=LIBC      Use the libc rand()/random() functions. (insecure!)")
  message("   SEED=ZERO      Use a zero seed. (insecure!)")
endif()

# Choose the pseudo-random number generator.
set(RELICRAND "HASH")

# Choose the pseudo-random number generator.
set(RELICSEED "UDEV")
