#!/usr/bin/env python
"""
The script builds OpenCV.framework for iOS.
The built framework is universal, it can be used to build app and run it on either iOS simulator or real device.

Usage:
    ./build_framework.py <outputdir>
    
By cmake conventions (and especially if you work with OpenCV SVN repository),
the output dir should not be a subdirectory of OpenCV source tree.
    
Script will create <outputdir>, if it's missing, and a few its subdirectories:
    
    <outputdir>
        build/
            iPhoneOS/
               [cmake-generated build tree for an iOS device target]
            iPhoneSimulator/
               [cmake-generated build tree for iOS simulator]
        OpenCV.framework/
            [the framework content]

The script should handle minor OpenCV updates efficiently
- it does not recompile the library from scratch each time.
However, OpenCV.framework directory is erased and recreated on each run.
"""

import glob, re, os, os.path, shutil, string, sys

def build_opencv(srcroot, buildroot, target, arch=None):
    "builds OpenCV for device or simulator"
    
    builddir = os.path.join(buildroot, target)
    if not os.path.isdir(builddir):
        os.makedirs(builddir)
    currdir = os.getcwd()
    os.chdir(builddir)
    # for some reason, if you do not specify CMAKE_BUILD_TYPE, it puts libs to "RELEASE" rather than "Release"
    cmakeargs = ("-GXcode " +
                "-DCMAKE_BUILD_TYPE=Release " +
                "-DCMAKE_TOOLCHAIN_FILE=%s/ios/cmake/Toolchains/Toolchain-%s_Xcode.cmake " +
                "-DBUILD_opencv_world=ON " +
                "-DCMAKE_INSTALL_PREFIX=install") % (srcroot, target)
    # if cmake cache exists, just rerun cmake to update OpenCV.xproj if necessary
    if os.path.isfile(os.path.join(builddir, "CMakeCache.txt")):
        os.system("cmake %s ." % (cmakeargs,))
    else:
        os.system("cmake %s %s" % (cmakeargs, srcroot))
    
    for wlib in [builddir + "/modules/world/UninstalledProducts/libopencv_world.a",
                 builddir + "/lib/Release/libopencv_world.a"]:
        if os.path.isfile(wlib):
            os.remove(wlib)

    arch_string = 'ARCHS="%s"' % arch if arch else ""
    os.system("xcodebuild -parallelizeTargets -jobs 8 -sdk %s -configuration Release %s -target ALL_BUILD" % (target.lower(), arch_string))
    os.system("xcodebuild -sdk %s -configuration Release %s -target install install" % (target.lower(), arch_string))
    os.chdir(currdir)
    
def put_framework_together(srcroot, dstroot):
    "constructs the framework directory after all the targets are built"
    
    # find the list of targets (basically, ["iPhoneOS", "iPhoneSimulator"])
    targetlist = glob.glob(os.path.join(dstroot, "build", "*"))
    targetlist = [os.path.basename(t) for t in targetlist]
    
    # set the current dir to the dst root
    currdir = os.getcwd()
    framework_dir = dstroot + "/opencv2.framework"
    if os.path.isdir(framework_dir):
        shutil.rmtree(framework_dir)
    os.makedirs(framework_dir)
    os.chdir(framework_dir)
    
    # determine OpenCV version (without subminor part)
    tdir0 = "../build/" + targetlist[0]
    cfg = open(tdir0 + "/cvconfig.h", "rt")
    for l in cfg.readlines():
        if l.startswith("#define  VERSION"):
            opencv_version = l[l.find("\"")+1:l.rfind(".")]
            break
    cfg.close()
    
    # form the directory tree
    dstdir = "Versions/A"
    os.makedirs(dstdir + "/Resources")

    # copy headers
    shutil.copytree(tdir0 + "/install/include/opencv2", dstdir + "/Headers")
    
    # make universal static lib
    wlist = " ".join(["../build/" + t + "/lib/Release/libopencv_world.a" for t in targetlist])
    os.system("lipo -create " + wlist + " -o " + dstdir + "/opencv2")
    
    # form Info.plist
    srcfile = open(srcroot + "/ios/Info.plist.in", "rt")
    dstfile = open(dstdir + "/Resources/Info.plist", "wt")
    for l in srcfile.readlines():
        dstfile.write(l.replace("${VERSION}", opencv_version))
    srcfile.close()
    dstfile.close()
    
    # copy cascades
    # TODO ...
    
    # make symbolic links
    os.symlink(dstdir + "/Headers", "Headers")
    os.symlink(dstdir + "/Resources", "Resources")
    os.symlink(dstdir + "/opencv2", "opencv2")
    os.symlink("A", "Versions/Current")
        


def lipo_static_libraries(srcroot, dstroot, libs):
    universal_dir = os.path.join(dstroot, "universal")
    try:
        os.makedirs(os.path.join(universal_dir, "lib"))
        os.makedirs(os.path.join(universal_dir, "include", "opencv2"))
    except OSError:
        pass

    armv7_dir = os.path.join(dstroot, "build", "armv7", "iPhoneOS", "lib", "Release")
    armv7s_dir = os.path.join(dstroot, "build", "armv7s", "iPhoneOS", "lib", "Release")
    for lib in libs:
        libname = "libopencv_{l}.a".format(l=lib)
        os.system("/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/lipo -create {v7}/{l} -arch armv7s {v7s}/{l} -output {u}/lib/{l}".format(v7=armv7_dir, v7s=armv7s_dir, u=universal_dir, l=libname))

    include_dir = os.path.join(dstroot, "build", "armv7", "iPhoneOS", "install", "include", "opencv2")
    for lib in libs:
        dst = os.path.join(universal_dir, "include", "opencv2", lib)
        try:
            shutil.rmtree(dst)
        except OSError:
            pass
        shutil.copytree(os.path.join(include_dir, lib), dst)

    shutil.copyfile(os.path.join(include_dir, "opencv.hpp"), os.path.join(universal_dir, "include", "opencv2", "opencv.hpp"))

    print "WROTE TO", universal_dir


def build_framework(srcroot, dstroot):
    "main function to do all the work"
    
    # for target in ["iPhoneOS", "iPhoneSimulator"]:
    #     build_opencv(srcroot, os.path.join(dstroot, "build"), target)
    for arch in ("armv7", "armv7s"):
        build_opencv(srcroot, os.path.join(dstroot, "build", arch), "iPhoneOS", arch=arch)

    # put_framework_together(srcroot, dstroot)
    libs = ["core", "imgproc"]
    lipo_static_libraries(srcroot, dstroot, libs)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage:\n\t./build_framework.py <outputdir>\n\n"
        sys.exit(0)
    
    build_framework(os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), "..")), os.path.abspath(sys.argv[1]))
