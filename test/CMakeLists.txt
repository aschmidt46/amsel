# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(antons-nes-emuTests LANGUAGES CXX)

# ---- Tests ----

add_executable(antons-nes-emu_test source/antons-nes-emu_test.cpp)
target_link_libraries(antons-nes-emu_test PRIVATE antons-nes-emu_lib)
target_compile_features(antons-nes-emu_test PRIVATE cxx_std_20)

add_test(NAME antons-nes-emu_test COMMAND antons-nes-emu_test)

# ---- End-of-file commands ----

add_folders(Test)
