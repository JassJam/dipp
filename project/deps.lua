add_requires("boost 1.86.0", {
    debug = is_mode("debug"),
    configs = {
        cmake = false,
        filesystem = true,
        test = true
    }
})

add_requires("benchmark")
