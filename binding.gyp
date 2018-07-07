{
    "targets": 
    [
        {
            "target_name":  "signaling-client",
            "sources":      [ "src/signaling_client.cpp", "src/addon.cc" ],
            "include_dirs": [ "<!(node -e \"require('nan')\")" ],
            "libraries":    ["-Wl,-rpath,libagorasig.so", "-lagorasig"],
            "cflags":       [ "-std=c++11" ]
        }
    ]
}
