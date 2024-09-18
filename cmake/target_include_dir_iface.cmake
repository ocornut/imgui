function(target_include_dir_iface target visibility build-inc install-inc)
    set(args ${ARGN})
    target_include_directories(
        ${target}
        ${visibility}
        $<BUILD_INTERFACE:${build-inc}>
        $<INSTALL_INTERFACE:${install-inc}>
        ${args}
    )
endfunction()
