
target("dipp")
    set_kind("headeronly")

    add_headerfiles(os.projectdir() .. "/include/dipp/**.hpp")
    add_includedirs(os.projectdir() .. "/include", {public = true})

    add_filegroups("dipp", {rootdir = os.projectdir()})
target_end()
