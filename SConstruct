import platform
import os
import shutil
import sys
from subprocess import call

def add_option( name, help, nargs, contributesToVariantDir,
                dest=None, default = None, type="string", choices=None ):

    if dest is None:
        dest = name

    AddOption( "--" + name , 
               dest=dest,
               type=type,
               nargs=nargs,
               action="store",
               choices=choices,
               default=default,
               help=help )

def get_option( name ):
    return GetOption( name )
  
def _has_option( name ):
    x = get_option( name )
    if x is None:
        return False

    if x == False:
        return False

    if x == "":
        return False

    return True

def has_option( name ):
    x = _has_option(name)

    return x

def remove_dir(path):
    if os.path.exists(path):
        shutil.rmtree(path)

###
### Options
###
add_option( "64" ,       "whether to force 64 bit" , 0 , True , "force64", False)
add_option( "32" ,       "whether to force 32 bit" , 0 , True , "force32", False)
add_option( "release" ,  "build in release mode"   , 0 , True)
add_option( "rebuild" ,  "rebuild target"          , 0 , True, None, False)
add_option( "check" , "do not build anything, just print check summary" , 0 , True, None, False)

check = has_option("check")


###
### "Targets"
###
add_option( "all" ,        "build everything, step by step" , 0 , True, None, False)
add_option( "mongodb" ,    "build only mongodb" ,             0 , True, None, False)
add_option( "qjson" ,      "build only qjson" ,               0 , True, None, False)
add_option( "qscintilla" , "build only qscintilla" ,          0 , True, None, False)

all_target = has_option("all")
robomongo_target = False
mongodb_target = has_option("mongodb")
qjson_taget = has_option("qjson")
qscintilla_target = has_option("qscintilla")

if all_target:
    mongodb_target = True
    qjson_taget = True
    qscintilla_target = True
    robomongo_target = True

if not all_target and not mongodb_target and not qjson_taget and not qscintilla_target:
    robomongo_target = True

target_str = ""

if all_target:
    target_str += "all "
else:
    if mongodb_target:
        target_str += "mongodb "
    if qjson_taget:
        target_str += "qjson "
    if qscintilla_target: 
        target_str += "qscintilla "
    if robomongo_target:
        target_str += "robomongo "

###
### Setup
###
num_jobs = GetOption('num_jobs')
clean = GetOption('clean')
rebuild = has_option("rebuild")
build = True

if clean:
    build = False

if rebuild:
    clean = True
    build = True

##
## OS
##
sys_platform = sys.platform

def is_linux():
    return sys.platform.startswith('linux')

def is_windows():
    return sys_platform == "windows"

def is_darwin():
    return sys_platform == "darwin"

##
## CPU architecture
##
is_x64 = has_option("force64")
is_x32 = has_option("force32")

current_arch = platform.machine()

if current_arch == 'x86_64':
    cpu_arch = 'amd64'
else:
    cpu_arch = 'i386'
    
if is_x64: 
   cpu_arch = 'amd64'

if is_x32:
   cpu_arch = 'i386'
   
cpu_arch_bits = 32 if cpu_arch == 'i386' else 64
  
##  
## Build mode
##
is_debug = has_option("debug")
is_release = has_option("release")

build_mode = 'debug'

if is_release:
    build_mode = 'release'

##
## Root dir
##
root_dir = Dir('.').abspath

##
## MongoDB third party dir
##
mongo_dir = os.path.join(root_dir, 'src', 'third-party', 'mongodb')
mongo_build_dir = os.path.join(mongo_dir, 'build')
qscintilla_dir = os.path.join(root_dir, 'src', 'third-party', 'qscintilla')
qscintilla_build_dir = os.path.join(qscintilla_dir, 'target')
qjson_dir = os.path.join(root_dir, 'src', 'third-party', 'qjson')
qjson_build_dir = os.path.join(qjson_dir, 'target')


def build_out_path(root_path):
    return os.path.join(root_path, "%(os)s-%(arch)s-%(mode)s" % { 'os' : sys_platform, 'arch' : cpu_arch_bits, 'mode' : build_mode })

##
## Summary
##
print "--------------------------------------------"
print "Build architecture: %s" % cpu_arch
print "Build mode: %s" % build_mode
print "Targets: %s" % target_str
print "Perform cleanup: %s" % str(clean)
print "Check: %s" % str(check)
#print "--------------------------------------------"

#print "Hello " + mongo_dir + " " + str(x64)

## 
## Build MongoDB
##
if mongodb_target:
    os.chdir(mongo_dir)
    mongodb_build_mode = "d" if build_mode == "debug" else "release"
    mongodb_build_platform = sys_platform
    mongodb_scons_command = "scons mongod mongoclient --usesm --%(mode)s --%(arch)s -j%(jobs)s" % \
        { 'mode' : mongodb_build_mode, 'arch' : cpu_arch_bits, 'jobs' : num_jobs }
    
    
    mongodb_out_dir = os.path.join(mongo_build_dir, mongodb_build_platform, str(cpu_arch_bits), mongodb_build_mode, "usesm")

    print "> MongoDB build command: " + mongodb_scons_command
    print "> MongoDB out dir: " + mongodb_out_dir
    
    if not check: # perform action
        
        if clean:
            remove_dir(mongodb_out_dir)
        
        if build:
            # build only if out folder not exists yet
            if not os.path.exists(mongodb_out_dir):
                return_code = call(mongodb_scons_command, shell=True)
                print return_code
            else:
                print "> MongoDB already exists. Skipping this step (use --rebuild to force)"
 

 
##
## Build QScintilla
##
if qscintilla_target:
    qscintilla_out_dir = build_out_path(qscintilla_build_dir)
    print "> QScintilla out dir: " + qscintilla_out_dir
    
    qscintilla_pro_path = os.path.join(qscintilla_dir, 'Qt4Qt5', 'qscintilla.pro')
    print "> QScintilla project file path: " + qscintilla_pro_path
    
    qscintilla_qmake_args = ''
    
    if is_linux() and is_debug:
        qscintilla_qmake_args = "-r -spec linux-g++ CONFIG+=debug CONFIG+=declarative_debug"
    
    if is_linux() and is_release:
        qscintilla_qmake_args = "-r -spec linux-g++ CONFIG+=release"
        
    if is_darwin() and is_debug:
        qscintilla_qmake_args = "-r -spec macx-g++ CONFIG+=debug CONFIG+=declarative_debug"
    
    if is_darwin() and is_release:
        qscintilla_qmake_args = "-r -spec macx-g++ CONFIG+=release"
        
    if is_windows() and is_debug:
        qscintilla_qmake_args = "-r -spec win32-msvc2010 \"CONFIG+=debug\" \"CONFIG+=declarative_debug\""
        
    if is_windows() and is_release:
        qscintilla_qmake_args = "-r -spec win32-msvc2010 \"CONFIG+=release\""
        
    print "> QScintilla qmake args: " + qscintilla_qmake_args
    
    qscintilla_qmake_command = "qmake %s %s" % (qscintilla_pro_path, qscintilla_qmake_args)
    
    if not check:
        
        if clean:
            remove_dir(qscintilla_out_dir)
        
        if build:
            # build only if out folder does not exist yet
            if not os.path.exists(qscintilla_out_dir):  
                os.makedirs(qscintilla_out_dir)
                os.chdir(qscintilla_out_dir)            
                return_code = call(qscintilla_qmake_command, shell=True)
                
                if return_code == 0:
                    print "(*) QScintilla make file generated successfully"
                else:
                    print "(!) Error when running qmake."
                
                return_code = call("make -w", shell=True)

                if return_code == 0:
                    print "(*) QScintilla compiled successfully"
                else:
                    print "(!) Error when compiling QScintilla"
                
            else:
                print "> QScintilla already exists. Skipping this step (use --rebuild to force)"
            
if qjson_taget:
    qjson_out_dir = build_out_path(qjson_build_dir)
    print "> QJSon out dir: " + qjson_out_dir
    
    qjson_qmake_project_path = os.path.join(qjson_dir, "qjson.pro")
    print "> QJSon project file path: " + qjson_qmake_project_path
    
    qjson_qmake_args = ''
    
    if is_linux() and is_debug:
        qjson_qmake_args = "-r -spec linux-g++ CONFIG+=debug CONFIG+=declarative_debug"
    
    if is_linux() and is_release:
        qjson_qmake_args = "-r -spec linux-g++ CONFIG+=release"
        
    if is_darwin() and is_debug:
        qjson_qmake_args = "-r -spec macx-g++ CONFIG+=debug CONFIG+=declarative_debug"
    
    if is_darwin() and is_release:
        qjson_qmake_args = "-r -spec macx-g++ CONFIG+=release"
        
    if is_windows() and is_debug:
        qjson_qmake_args = "-r -spec win32-msvc2010 \"CONFIG+=debug\" \"CONFIG+=declarative_debug\""
        
    if is_windows() and is_release:
        qjson_qmake_args = "-r -spec win32-msvc2010 \"CONFIG+=release\""
        
    print "> QJson qmake args: " + qjson_qmake_args
    
    qjson_qmake_command = "qmake %s %s" % (qjson_qmake_project_path, qjson_qmake_args)
    
    if not check:
        
        if clean:
            remove_dir(qjson_out_dir)
        
        if build:
            # build only if out folder does not exist yet
            if not os.path.exists(qjson_out_dir):  
                os.makedirs(qjson_out_dir)
                os.chdir(qjson_out_dir)            
                return_code = call(qjson_qmake_command, shell=True)
                
                if return_code == 0:
                    print "(*) QJson make file generated successfully"
                else:
                    print "(!) Error when running qmake."
                
                return_code = call("make -w", shell=True)

                if return_code == 0:
                    print "(*) QJSon compiled successfully"
                else:
                    print "(!) Error when compiling QJson"
                
            else:
                print "> QJSon already exists. Skipping this step (use --rebuild to force)"    
            
print "--------------------------------------------"