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

##
## OS
##
sys_platform = sys.platform

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
mongo_dir = root_dir + '/src/third-party/mongodb'
mongo_build_dir = mongo_dir + '/build'

##
## Summary
##
print "--------------------------------------------"
print "Build architecture: %s" % cpu_arch
print "Build mode: %s" % build_mode
print "Targets: %s" % target_str
print "Perform cleanup: %s" % str(clean)
print "Check: %s" % str(check)
print "--------------------------------------------"

#print "Hello " + mongo_dir + " " + str(x64)

if not check:
    ## 
    ## Build MongoDB
    ##
    os.chdir(mongo_dir)
    mongodb_build_mode = "d" if build_mode == "debug" else "release"
    mongodb_build_platform = sys_platform
    mongodb_scons_command = "scons mongod mongoclient --usesm --%(mode)s --%(arch)s -j%(jobs)s" % \
	{ 'mode' : mongodb_build_mode, 'arch' : cpu_arch_bits, 'jobs' : num_jobs }
     
    mongodb_out_dir = mongo_build_dir + "/%(platform)s/%(arch)s/%(mode)s/usesm" % \
        { 'platform' : mongodb_build_platform, 'mode' : mongodb_build_mode, 'arch' : cpu_arch_bits }

    print mongodb_scons_command
    print mongodb_out_dir

    if clean:
	shutil.rmtree(mongo_build_dir)
    else:
	return_code = call(mongodb_scons_command, shell=True)
	print return_code
 