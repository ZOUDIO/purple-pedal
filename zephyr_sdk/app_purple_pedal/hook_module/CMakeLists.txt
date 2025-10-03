if(CONFIG_MCUBOOT AND CONFIG_MCUBOOT_ACTION_HOOKS)
  # Append source file which outputs the current mode of mcuboot
  target_sources(app PRIVATE src/hook.c)
endif()