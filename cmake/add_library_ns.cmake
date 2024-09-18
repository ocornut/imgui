function(add_library_ns target_ns target_name)
    # Forward all arguments after the target_name
    set(args ${ARGN})
    set(target_fullname ${target_ns}-${target_name})

    # Call the original add_library command with the forwarded arguments
    add_library(${target_fullname} ${args})

    # Alias it properly
    add_library(${target_ns}::${target_name} ALIAS ${target_fullname})

    # Export name unambiguate
    set_target_properties(${target_fullname} PROPERTIES EXPORT_NAME ${target_name})
endfunction()
