include_guard(DIRECTORY)
include(CMakePackageConfigHelpers)

# ──────────────────────────────────────────────────────────────────────────────
# register_package_config(
#     NAME            <PackageName>          # used for file names + find_package()
#     VERSION         <x.y.z>               # defaults to PROJECT_VERSION
#     [NAMESPACE      <ns>]                 # import namespace, e.g. "MyProject"
#     [EXPORT_SET     <set>]                # name passed to register_*() targets
#                                           # defaults to <NAME>Targets
#     [COMPATIBILITY  AnyNewerVersion       # default
#                   | SameMajorVersion
#                   | SameMinorVersion
#                   | ExactVersion]
#     [INSTALL_DESTINATION <dir>]           # default: lib/cmake/<NAME>
#     [DEPENDENCIES   <pkg> …]              # written as find_dependency() calls
#                                           # format: "PkgName" or "PkgName 1.2"
#                                           # or "PkgName 1.2 EXACT"
#     [EXTRA_CONFIG_CONTENT <string>]       # verbatim CMake appended to the
#                                           # generated Config.cmake
# )
#
# What it does:
#   1. Generates <NAME>Config.cmake      via configure_package_config_file()
#   2. Generates <NAME>ConfigVersion.cmake via write_basic_package_version_file()
#   3. Installs both files + the export set into INSTALL_DESTINATION
#
# After calling this once, consumers can simply do:
#   find_package(<NAME> REQUIRED)
# ──────────────────────────────────────────────────────────────────────────────
function(register_package_config)
    cmake_parse_arguments(PARSE_ARGV 0 ARG
        ""
        "NAME;VERSION;NAMESPACE;EXPORT_SET;COMPATIBILITY;INSTALL_DESTINATION;EXTRA_CONFIG_CONTENT"
        "DEPENDENCIES"
    )

    # ── Validate required args ────────────────────────────────────────────────
    if(NOT DEFINED ARG_NAME)
        message(FATAL_ERROR "register_package_config: NAME is required")
    endif()

    # ── Defaults ──────────────────────────────────────────────────────────────
    if(NOT DEFINED ARG_VERSION)
        set(ARG_VERSION "${PROJECT_VERSION}")
    endif()

    if(NOT DEFINED ARG_EXPORT_SET)
        set(ARG_EXPORT_SET "${ARG_NAME}Targets")
    endif()

    if(NOT DEFINED ARG_COMPATIBILITY)
        set(ARG_COMPATIBILITY AnyNewerVersion)
    endif()

    if(NOT DEFINED ARG_INSTALL_DESTINATION)
        set(ARG_INSTALL_DESTINATION "lib/cmake/${ARG_NAME}")
    endif()

    set(_ns "")
    if(DEFINED ARG_NAMESPACE)
        set(_ns "NAMESPACE" "${ARG_NAMESPACE}::")
    endif()

    # ── Build find_dependency() block ─────────────────────────────────────────
    # Each entry in DEPENDENCIES may be:
    #   "Foo"              → find_dependency(Foo)
    #   "Foo 1.2"          → find_dependency(Foo 1.2)
    #   "Foo 1.2 EXACT"    → find_dependency(Foo 1.2 EXACT)
    set(_dep_block "")
    foreach(_dep IN LISTS ARG_DEPENDENCIES)
        string(APPEND _dep_block "find_dependency(${_dep})\n")
    endforeach()

    # ── Generate Config.cmake in the build tree ───────────────────────────────
    # We write a temporary .cmake.in and let configure_package_config_file
    # handle the @PACKAGE_*@ substitutions correctly.
    set(_config_in "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}Config.cmake.in")
    set(_config_out "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}Config.cmake")

    set(_config_in_content
"@PACKAGE_INIT@

include(CMakeFindDependencyMacro)
${_dep_block}
include(\"\${CMAKE_CURRENT_LIST_DIR}/${ARG_EXPORT_SET}Targets.cmake\")
${ARG_EXTRA_CONFIG_CONTENT}
check_required_components(${ARG_NAME})
")

    file(WRITE "${_config_in}" "${_config_in_content}")

    configure_package_config_file(
        "${_config_in}"
        "${_config_out}"
        INSTALL_DESTINATION "${ARG_INSTALL_DESTINATION}"
    )

    # ── Version file ──────────────────────────────────────────────────────────
    set(_version_out "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}ConfigVersion.cmake")

    write_basic_package_version_file(
        "${_version_out}"
        VERSION       "${ARG_VERSION}"
        COMPATIBILITY "${ARG_COMPATIBILITY}"
    )

    # ── Install export set ────────────────────────────────────────────────────
    # Only install the export if register_*() hasn't already done it.
    # (register_*() tracks emitted sets in _REGISTER_EXPORTED_SETS.)
    get_property(_already_exported GLOBAL PROPERTY _REGISTER_EXPORTED_SETS)
    if(NOT ARG_EXPORT_SET IN_LIST _already_exported)
        install(EXPORT "${ARG_EXPORT_SET}"
            FILE        "${ARG_EXPORT_SET}Targets.cmake"
            ${_ns}
            DESTINATION "${ARG_INSTALL_DESTINATION}"
        )
        # Mark as done so register_*() won't duplicate it
        list(APPEND _already_exported "${ARG_EXPORT_SET}")
        set_property(GLOBAL PROPERTY _REGISTER_EXPORTED_SETS "${_already_exported}")
    else()
        # Export was installed by register_*() without a destination matching
        # ARG_INSTALL_DESTINATION — reinstall with the correct path.
        # (Harmless duplicate on identical destination; CMake deduplicates.)
        install(EXPORT "${ARG_EXPORT_SET}"
            FILE        "${ARG_EXPORT_SET}Targets.cmake"
            ${_ns}
            DESTINATION "${ARG_INSTALL_DESTINATION}"
        )
    endif()

    # ── Install config + version files ────────────────────────────────────────
    install(FILES
        "${_config_out}"
        "${_version_out}"
        DESTINATION "${ARG_INSTALL_DESTINATION}"
    )

    message(STATUS
        "[register_package_config] '${ARG_NAME}' v${ARG_VERSION} "
        "(${ARG_COMPATIBILITY}) → ${ARG_INSTALL_DESTINATION}"
    )
endfunction()