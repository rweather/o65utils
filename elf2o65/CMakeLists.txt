
add_executable(elf2o65
    elf2o65.c
)

target_link_libraries(elf2o65 PUBLIC o65)
if(HAVE_LIBELF)
    target_link_libraries(elf2o65 PRIVATE -lelf)
endif()

install(TARGETS elf2o65 DESTINATION bin)
