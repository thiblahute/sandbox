project('test', 'cpp', 'c')

deps = [dependency('glib-2.0'), dependency('gstreamer-1.0')]

foreach f: ['v4l2src_tee_appsink_videosink.c']
    e = executable(f.split('.')[0], f, dependencies: deps)
    test(f, e)
endforeach
