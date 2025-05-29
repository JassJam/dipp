local boost_lib_version = "1.88.0"
local boost_packages = {
    "cmake",
    "leaf"
}

if is_config("test", true) then
    table.insert(boost_packages, "test")
end

add_requires("boost[".. table.concat(boost_packages, ",") .."] " .. boost_lib_version)

if is_config("benchmark", true) then
    add_requires("benchmark")
    add_requires("kangaru")
    add_requires("fruit")
end