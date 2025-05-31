includes("project/options.lua")
add_rules("mode.debug", "mode.release")

--

set_languages("$(cpp-version)")
add_extrafiles(".clang-format")

--

includes("project/packages.lua")
includes("project/project.lua")

if is_config("test", true) then
    includes("project/tests.lua")
end

if is_config("benchmark", true) then
    includes("project/benchmarks.lua")
end