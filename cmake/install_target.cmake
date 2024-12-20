function(install_target_and_headers ns name)

set(target_name ${ns}-${name})

install(FILES
${ARGN}
DESTINATION include)

install(TARGETS ${target_name}
    EXPORT ${target_name}-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install(EXPORT ${target_name}-targets
    FILE ${target_name}-targets.cmake
    NAMESPACE ${ns}::
    DESTINATION lib/cmake/${ns})
endfunction()
