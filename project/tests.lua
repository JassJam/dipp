-- opts:
--  opts.name: the project test name
--  opts.path: the path to the test files
local function add_test(opts)
    local file_path = os.projectdir() .. "/tests/" .. opts.path
    target(opts.name)
        set_group("tests")
        set_kind("binary")
        add_tests("default")

        add_deps("dipp")
        add_packages("boost")

        add_files(file_path .. "/**.cpp")
        add_headerfiles(file_path .. "/**.hpp")

        add_filegroups(opts.name, {rootdir = file_path})
    target_end()
end

local tests_dir = os.projectdir() .. "/tests/"
-- for all folder in the tests directory
for _, dir in ipairs(os.dirs(tests_dir .. "*/")) do
    if os.isdir(dir) then
        local test_name = path.filename(dir)
        add_test({name = "test_" .. test_name, path = test_name})
    end
end
