
-- in lua, iterate over the folders in the directory

-- opts:
--  opts.name: the project test name
--  opts.path: the path to the test files
local function add_test(opts)
    local file_path = os.projectdir() .. "/tests/" .. opts.path
    target(opts.name)
        set_group("tests")
        set_kind("binary")
        add_deps("dipp")
        add_packages("boost")

        add_files(file_path .. "/**.cpp")
        add_headerfiles(file_path .. "/**.hpp")

        add_filegroups(opts.name, {rootdir = file_path})
    target_end()
end

add_test({name = "test_basic_services", path = "basic_services"})
add_test({name = "test_services_invoke", path = "services_invoke"})