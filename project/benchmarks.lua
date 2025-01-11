
-- in lua, iterate over the folders in the directory

add_requires("kangaru")
add_requires("fruit")

-- opts:
--  opts.name: the project test name
--  opts.path: the path to the test files
local function add_benchamrk(opts)
    local file_path = os.projectdir() .. "/benchmarks/" .. opts.path
    target(opts.name)
        set_group("benchmarks")
        set_kind("binary")
        add_deps("dipp")

        add_packages("boost")
        add_packages("benchmark")
        
        add_packages("kangaru")
        add_packages("fruit")

        add_files(file_path .. "/**.cpp")
        add_headerfiles(file_path .. "/**.hpp")

        add_filegroups(opts.name, {rootdir = file_path})
    target_end()
end

add_benchamrk({name = "benchmark_basic_services", path = "basic_services"})