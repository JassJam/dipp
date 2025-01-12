option("test")
    set_default(false)
option_end()

option("benchmark")
    set_default(false)
option_end()

add_rules("mode.debug", "mode.release")

--

set_languages("c++20")

add_extrafiles(".clang-format")
add_extrafiles(".clang-tidy")

includes("project/project.lua")

if is_config("test", true) then
    includes("project/tests.lua")
end

if is_config("benchmark", true) then
    includes("project/benchmarks.lua")
end