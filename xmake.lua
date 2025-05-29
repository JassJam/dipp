option("test")
    set_default(false)
option_end()

option("benchmark")
    set_default(false)
option_end()

option("exceptions")
    set_default(true)
option_end()

option("cpp-version")
    set_default("c++20")
option_end()

add_rules("mode.debug", "mode.release")

--

set_languages("$(cpp-version)")

add_extrafiles(".clang-format")

includes("project/packages.lua")
includes("project/project.lua")

if is_config("test", true) then
    includes("project/tests.lua")
end

if is_config("benchmark", true) then
    includes("project/benchmarks.lua")
end