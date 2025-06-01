
target("dipp")
    if is_config("exceptions", false) then
        add_defines("DIPP_NO_EXCEPTIONS", {public = true})
    end

    set_kind("headeronly")

    add_headerfiles(os.projectdir() .. "/include/dipp/*.hpp", {prefixdir = "dipp"})
    add_headerfiles(os.projectdir() .. "/include/dipp/details/*.hpp", {prefixdir = "dipp/details"})
    add_headerfiles(os.projectdir() .. "/include/dipp/details/errors/*.hpp", {prefixdir = "dipp/details/errors"})
    add_includedirs(os.projectdir() .. "/include", {public = true})

    add_filegroups("dipp", {rootdir = os.projectdir()})
    add_result_packages()

    if is_config("error-type", "result") then
        add_defines("DIPP_USE_RESULT", {public = true})
    end
target_end()
