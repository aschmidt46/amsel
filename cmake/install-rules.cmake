install(
    TARGETS antons-nes-emu_exe
    RUNTIME COMPONENT antons-nes-emu_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
