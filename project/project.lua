
target("dipp")
    set_kind("headeronly")

    add_headerfiles(os.projectdir() .. "/include/(dipp/**.hpp)")
    add_includedirs(os.projectdir() .. "/include", {public = true})

    add_filegroups("dipp", {rootdir = os.projectdir()})
    add_result_packages()

    if is_config("error-type", "result") then
        add_defines("DIPP_USE_RESULT", {public = true})
    end
target_end()
