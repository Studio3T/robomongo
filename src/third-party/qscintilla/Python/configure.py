# This script configures QScintilla for PyQt v4.10 and later.  It will fall
# back to the old script if an earlier version of PyQt is found.
#
# Copyright (c) 2012 Riverbank Computing Limited <info@riverbankcomputing.com>
# 
# This file is part of QScintilla.
# 
# This file may be used under the terms of the GNU General Public
# License versions 2.0 or 3.0 as published by the Free Software
# Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
# included in the packaging of this file.  Alternatively you may (at
# your option) use any later version of the GNU General Public
# License if such license has been publicly approved by Riverbank
# Computing Limited (or its successors, if any) and the KDE Free Qt
# Foundation. In addition, as a special exception, Riverbank gives you
# certain additional rights. These rights are described in the Riverbank
# GPL Exception version 1.1, which can be found in the file
# GPL_EXCEPTION.txt in this package.
# 
# If you are unsure which license is appropriate for your use, please
# contact the sales department at sales@riverbankcomputing.com.
# 
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


import sys
import os
import glob
import optparse

try:
    import sysconfig
except ImportError:
    from distutils import sysconfig


# Initialise the constants.
SIP_MIN_VERSION = '4.12.0'

# This must be kept in sync with qscintilla.pro, Qt4Qt5/application.pro and
# Qt4Qt5/designer.pro.
QSCI_API_MAJOR = 9


def error(msg):
    """ Display an error message and terminate.  msg is the text of the error
    message.
    """

    sys.stderr.write(format("Error: " + msg) + "\n")
    sys.exit(1)


def inform(msg):
    """ Display an information message.  msg is the text of the error message.
    """

    sys.stdout.write(format(msg) + "\n")


def format(msg, left_margin=0, right_margin=78):
    """ Format a message by inserting line breaks at appropriate places.  msg
    is the text of the message.  left_margin is the position of the left
    margin.  right_margin is the position of the right margin.  Returns the
    formatted message.
    """

    curs = left_margin
    fmsg = " " * left_margin

    for w in msg.split():
        l = len(w)
        if curs != left_margin and curs + l > right_margin:
            fmsg = fmsg + "\n" + (" " * left_margin)
            curs = left_margin

        if curs > left_margin:
            fmsg = fmsg + " "
            curs = curs + 1

        fmsg = fmsg + w
        curs = curs + l

    return fmsg


class HostPythonConfiguration:
    """ A container for the host Python configuration. """

    def __init__(self):
        """ Initialise the configuration. """

        self.platform = sys.platform
        self.version = sys.hexversion >> 8

        if hasattr(sysconfig, 'get_path'):
            # The modern API.
            self.inc_dir = sysconfig.get_path('include')
            self.module_dir = sysconfig.get_path('platlib')
        else:
            # The legacy distutils API.
            self.inc_dir = sysconfig.get_python_inc(plat_specific=1)
            self.module_dir = sysconfig.get_python_lib(plat_specific=1)

        if sys.platform == 'win32':
            self.data_dir = sys.prefix
            self.lib_dir = sys.prefix + '\\libs'
        else:
            self.data_dir = sys.prefix + '/share'
            self.lib_dir = sys.prefix + '/lib'


class TargetQtConfiguration:
    """ A container for the target Qt configuration. """

    def __init__(self, qmake):
        """ Initialise the configuration.  qmake is the full pathname of the
        qmake executable that will provide the configuration.
        """

        pipe = os.popen(' '.join([qmake, '-query']))

        for l in pipe:
            l = l.strip()

            tokens = l.split(':', 1)
            if isinstance(tokens, list):
                if len(tokens) != 2:
                    error("Unexpected output from qmake: '%s'\n" % l)

                name, value = tokens
            else:
                name = tokens
                value = None

            name = name.replace('/', '_')

            setattr(self, name, value)

        pipe.close()


class TargetConfiguration:
    """ A container for the target configuration. """

    def __init__(self):
        """ Initialise the configuration with default values. """

        # Values based on the host Python configuration.
        py_config = HostPythonConfiguration()
        self.py_module_dir = py_config.module_dir
        self.py_inc_dir = py_config.inc_dir
        self.py_lib_dir = py_config.lib_dir
        self.py_platform = py_config.platform
        self.py_sip_dir = os.path.join(py_config.data_dir, 'sip')
        self.py_version = py_config.version
        self.sip_inc_dir = py_config.inc_dir

        # The default qmake spec.
        if self.py_platform == 'win32':
            if self.py_version >= 0x030300:
                self.qmake_spec = 'win32-msvc2010'
            elif self.py_version >= 0x020600:
                self.qmake_spec = 'win32-msvc2008'
            elif self.py_version >= 0x020400:
                self.qmake_spec = 'win32-msvc.net'
            else:
                self.qmake_spec = 'win32-msvc'
        else:
            # Use the Qt default.  (We may update it for MacOS/X later.)
            self.qmake_spec = ''

        # Remaining values.
        self.pyqt_sip_flags = ''
        self.pyqt_version = ''
        self.qmake = self._find_exe('qmake')
        self.sip = self._find_exe('sip')

        self.prot_is_public = (self.py_platform.startswith('linux') or self.py_platform == 'darwin')
        self.qscintilla_is_dll = (self.py_platform == 'win32')

        self.module_dir = os.path.join(py_config.module_dir, 'PyQt4')
        self.pyqt_sip_dir = os.path.join(self.py_sip_dir, 'PyQt4')
        self.qsci_sip_dir = self.pyqt_sip_dir

    def from_configuration_file(self, config_file):
        """ Initialise the configuration with values from a file.  config_file
        is the name of the configuration file.
        """

        inform("Reading configuration from %s..." % config_file)

        cfg = open(config_file)
        line_nr = 0

        for l in cfg:
            line_nr += 1
            l = l.strip()

            if len(l) == 0 or l[0] == '#':
                continue

            eq = l.find('=')
            if eq > 0:
                name = l[:eq - 1].rstrip()
                value = l[eq + 1:].lstrip()
            else:
                name = value = ''

            if name == '' or value == '':
                error("%s:%d: Invalid line." % (config_file, line_nr))

            default_value = getattr(self, name, None)
            if default_value is None:
                error(
                        "%s:%d: Unknown item: %s." % (config_file, line_nr,
                                name))

            if isinstance(default_value, int):
                if value.startswith('0x'):
                    value = int(value, 16)
                else:
                    value = int(value)

            setattr(self, name, value)

        cfg.close()

    def from_introspection(self, pyqt_package):
        """ Initialise the configuration by introspecting the system.
        pyqt_package is the name of the PyQt package we are building against.
        """

        if pyqt_package == 'PyQt5':
            try:
                from PyQt5 import QtCore
            except ImportError:
                error(
                        "Unable to import PyQt5.QtCore. Make sure PyQt5 is "
                        "installed.")
        else:
            try:
                from PyQt4 import QtCore
            except ImportError:
                error(
                        "Unable to import PyQt4.QtCore. Make sure PyQt4 is "
                        "installed.")

        inform("PyQt %s is being used." % QtCore.PYQT_VERSION_STR)
        inform("Qt %s is being used." % QtCore.QT_VERSION_STR)

        # See if we have a PyQt that embeds its configuration.
        try:
            pyqt_config = QtCore.PYQT_CONFIGURATION
        except AttributeError:
            pyqt_config = None

        if pyqt_config is None:
            # Fallback to the old configuration script.
            config_script = sys.argv[0].replace('configure', 'configure-old')
            args = [sys.executable, config_script] + sys.argv[1:]

            try:
                os.execv(sys.executable, args)
            except OSError:
                pass

            error("Unable to execute '%s'\n" % config_script)

        self.pyqt_sip_flags = pyqt_config['sip_flags']

    def get_qt_configuration(self, opts):
        """ Get the Qt configuration that can be extracted from qmake.  opts
        are the command line options.
        """

        try:
            qmake = opts.qmake
        except AttributeError:
            # Windows.
            qmake = None

        if qmake is not None:
            self.qmake = qmake
        elif self.qmake is None:
            # Under Windows qmake and the Qt DLLs must be on the system PATH
            # otherwise the dynamic linker won't be able to resolve the
            # symbols.  On other systems we assume we can just run qmake by
            # using its full pathname.
            if sys.platform == 'win32':
                error("Make sure you have a working Qt qmake on your PATH.")
            else:
                error(
                        "Make sure you have a working Qt qmake on your PATH "
                        "or use the --qmake argument to explicitly specify a "
                        "working Qt qmake.")

        # Query qmake.
        qt_config = TargetQtConfiguration(self.qmake)

        # The binary MacOS/X Qt installer defaults to XCode.  If this is what
        # we might have then use macx-clang (Qt v5) or macx-g++ (Qt v4).
        if sys.platform == 'darwin':
            try:
                # Qt v5.
                if qt_config.QMAKE_SPEC == 'macx-xcode':
                    # This will exist (and we can't check anyway).
                    self.qmake_spec = 'macx-clang'
                else:
                    # No need to explicitly name the default.
                    self.qmake_spec = ''
            except AttributeError:
                # Qt v4.
                self.qmake_spec = 'macx-g++'

        self.api_dir = qt_config.QT_INSTALL_DATA
        self.qsci_inc_dir = qt_config.QT_INSTALL_HEADERS
        self.qsci_lib_dir = qt_config.QT_INSTALL_LIBS

    def override_defaults(self, opts):
        """ Override the defaults from the command line.  opts are the command
        line options.
        """

        if opts.apidir is not None:
            self.api_dir = opts.apidir

        if opts.destdir is not None:
            self.module_dir = opts.destdir
        else:
            self.module_dir = os.path.join(self.py_module_dir,
                    opts.pyqt_package)

        if opts.qmakespec is not None:
            self.qmake_spec = opts.qmakespec

        if opts.prot_is_public is not None:
            self.prot_is_public = opts.prot_is_public

        if opts.qsci_inc_dir is not None:
            self.qsci_inc_dir = opts.qsci_inc_dir

        if opts.qsci_lib_dir is not None:
            self.qsci_lib_dir = opts.qsci_lib_dir

        if opts.sip_inc_dir is not None:
            self.sip_inc_dir = opts.sip_inc_dir

        if opts.pyqt_sip_dir is not None:
            self.pyqt_sip_dir = opts.pyqt_sip_dir
        else:
            self.pyqt_sip_dir = os.path.join(self.py_sip_dir,
                    opts.pyqt_package)

        if opts.qsci_sip_dir is not None:
            self.qsci_sip_dir = opts.qsci_sip_dir
        else:
            self.qsci_sip_dir = self.pyqt_sip_dir

        if opts.sip is not None:
            self.sip = opts.sip

        if opts.is_dll is not None:
            self.qscintilla_is_dll = opts.is_dll

    @staticmethod
    def _find_exe(exe):
        """ Find an executable, ie. the first on the path. """

        try:
            path = os.environ['PATH']
        except KeyError:
            path = ''

        if sys.platform == 'win32':
            exe = exe + '.exe'

        for d in path.split(os.pathsep):
            exe_path = os.path.join(d, exe)

            if os.access(exe_path, os.X_OK):
                return exe_path

        return None


def create_optparser(target_config):
    """ Create the parser for the command line.  target_config is the target
    configuration containing default values.
    """

    def store_abspath(option, opt_str, value, parser):
        setattr(parser.values, option.dest, os.path.abspath(value))

    def store_abspath_dir(option, opt_str, value, parser):
        if not os.path.isdir(value):
            raise optparse.OptionValueError("'%s' is not a directory" % value)
        setattr(parser.values, option.dest, os.path.abspath(value))

    def store_abspath_exe(option, opt_str, value, parser):
        if not os.access(value, os.X_OK):
            raise optparse.OptionValueError("'%s' is not an executable" % value)
        setattr(parser.values, option.dest, os.path.abspath(value))

    p = optparse.OptionParser(usage="python %prog [options]",
            version="2.7.2")

    p.add_option("--spec", dest='qmakespec', default=None, action='store',
            metavar="SPEC",
            help="pass -spec SPEC to qmake [default: %s]" % "don't pass -spec" if target_config.qmake_spec == '' else target_config.qmake_spec)
    p.add_option("--apidir", "-a", dest='apidir', type='string', default=None,
            action='callback', callback=store_abspath, metavar="DIR", 
            help="the QScintilla API file will be installed in DIR [default: "
                    "QT_INSTALL_DATA/qsci]")
    p.add_option("--configuration", dest='config_file', type='string',
            default=None, action='callback', callback=store_abspath,
            metavar="FILE",
            help="FILE defines the target configuration")
    p.add_option("--destdir", "-d", dest='destdir', type='string',
            default=None, action='callback', callback=store_abspath,
            metavar="DIR",
            help="install the QScintilla module in DIR [default: "
                    "%s]" % target_config.module_dir)
    p.add_option("--protected-is-public", dest='prot_is_public', default=None,
            action='store_true',
            help="enable building with 'protected' redefined as 'public' "
                    "[default: %s]" % target_config.prot_is_public)
    p.add_option("--protected-not-public", dest='prot_is_public',
            action='store_false',
            help="disable building with 'protected' redefined as 'public'")
    p.add_option("--pyqt", dest='pyqt_package', type='choice',
            choices=['PyQt4', 'PyQt5'], default='PyQt4', action='store',
            metavar="PyQtn",
            help="configure for PyQt4 or PyQt5 [default: PyQt4]")

    if sys.platform != 'win32':
        p.add_option("--qmake", "-q", dest='qmake', type='string',
                default=None, action='callback', callback=store_abspath_exe,
                metavar="FILE",
                help="the pathname of qmake is FILE [default: "
                        "%s]" % (target_config.qmake or "None"))

    p.add_option("--qsci-incdir", "-n", dest='qsci_inc_dir', type='string',
            default=None, action='callback', callback=store_abspath_dir,
            metavar="DIR",
            help="the directory containing the QScintilla Qsci header file "
                    "directory is DIR [default: QT_INSTALL_HEADERS]")
    p.add_option("--qsci-libdir", "-o", dest='qsci_lib_dir', type='string',
            default=None, action='callback', callback=store_abspath_dir,
            metavar="DIR",
            help="the directory containing the QScintilla library is DIR "
                    "[default: QT_INSTALL_LIBS]")
    p.add_option("--sip", dest='sip', type='string', default=None,
            action='callback', callback=store_abspath_exe, metavar="FILE",
            help="the pathname of sip is FILE [default: "
                    "%s]" % (target_config.sip or "None"))
    p.add_option("--sip-incdir", dest='sip_inc_dir', type='string',
            default=None, action='callback', callback=store_abspath_dir,
            metavar="DIR",
            help="the directory containing the sip.h header file file is DIR "
                    "[default: %s]" % target_config.sip_inc_dir)
    p.add_option("--pyqt-sipdir", dest='pyqt_sip_dir', type='string',
            default=None, action='callback', callback=store_abspath_dir,
            metavar="DIR",
            help="the directory containing the PyQt .sip files is DIR "
                    "[default: %s]" % target_config.pyqt_sip_dir)
    p.add_option("--qsci-sipdir", "-v", dest='qsci_sip_dir', type='string',
            default=None, action='callback', callback=store_abspath_dir,
            metavar="DIR",
            help="the QScintilla .sip files will be installed in DIR "
                    "[default: %s]" % target_config.qsci_sip_dir)

    p.add_option("--concatenate", "-c", dest='concat', default=False,
            action='store_true', 
            help="concatenate the C++ source files")
    p.add_option("--concatenate-split", "-j", dest='split', type='int',
            default=1, metavar="N",
            help="split the concatenated C++ source files into N pieces "
                    "[default: 1]")
    p.add_option("--static", "-k", dest='static', default=False,
            action='store_true',
            help="build the QScintilla module as a static library")
    p.add_option("--no-docstrings", dest='no_docstrings', default=False,
            action='store_true',
            help="disable the generation of docstrings")
    p.add_option("--trace", "-r", dest='tracing', default=False,
            action='store_true',
            help="build the QScintilla module with tracing enabled")
    p.add_option("--no-dll", "-s", dest='is_dll', default=None,
            action='store_false',
            help="QScintilla is a static library and not a Windows DLL")
    p.add_option("--debug", "-u", default=False, action='store_true',
            help="build the QScintilla module with debugging symbols")
    p.add_option("--no-timestamp", "-T", dest='no_timestamp', default=False,
            action='store_true',
            help="suppress timestamps in the header comments of generated "
                    "code [default: include timestamps]")

    return p


def inform_user(target_config):
    """ Tell the user the values that are going to be used.  target_config is
    the target configuration.
    """

    inform("The sip executable is %s." % target_config.sip)

    inform("The QScintilla module will be installed in %s." % target_config.module_dir)

    if target_config.prot_is_public:
        inform("The QScintilla module is being built with 'protected' "
                "redefined as 'public'.")

    inform("The QScintilla .sip files will be installed in %s." %
            target_config.qsci_sip_dir)

    inform("The QScintilla API file will be installed in %s." %
                os.path.join(target_config.api_dir, 'api', 'python'))


def check_qscintilla(target_config):
    """ See if QScintilla can be found and what its version is.  target_config
    is the target configuration.
    """

    # Find the QScintilla header files.
    sciglobal = os.path.join(target_config.qsci_inc_dir, 'Qsci', 'qsciglobal.h')

    if not os.access(sciglobal, os.F_OK):
        error("Qsci/qsciglobal.h could not be found in %s. If QScintilla is installed then use the --qsci-incdir argument to explicitly specify the correct directory." % target_config.qsci_inc_dir)

    # Get the QScintilla version string.
    sciversstr = read_define(sciglobal, 'QSCINTILLA_VERSION_STR')
    if sciversstr is None:
        error(
                "The QScintilla version number could not be determined by "
                "reading %s." % sciglobal)

    if not glob.glob(os.path.join(target_config.qsci_lib_dir, '*qscintilla2*')):
        error("The QScintilla library could not be found in %s. If QScintilla is installed then use the --qsci-libdir argument to explicitly specify the correct directory." % target_config.qsci_lib_dir)

    # Because we include the Python bindings with the C++ code we can
    # reasonably force the same version to be used and not bother about
    # versioning.
    if sciversstr != '2.7.2':
        error("QScintilla %s is being used but the Python bindings 2.7.2 are being built. Please use matching versions." % sciversstr)

    inform("QScintilla %s is being used." % sciversstr)


def read_define(filename, define):
    """ Read the value of a #define from a file.  filename is the name of the
    file.  define is the name of the #define.  None is returned if there was no
    such #define.
    """

    f = open(filename)

    for l in f:
        wl = l.split()
        if len(wl) >= 3 and wl[0] == "#define" and wl[1] == define:
            # Take account of embedded spaces.
            value = ' '.join(wl[2:])[1:-1]
            break
    else:
        value = None

    f.close()

    return value


def sip_flags(target_config):
    """ Return the SIP flags.  target_config is the target configuration. """

    # Get the flags used for the main PyQt module.
    flags = target_config.pyqt_sip_flags.split()

    # Generate the API file.
    flags.append('-a')
    flags.append('QScintilla2.api')

    # Add PyQt's .sip files to the search path.
    flags.append('-I')
    flags.append(target_config.pyqt_sip_dir)

    return flags


def generate_code(target_config, opts):
    """ Generate the code for the QScintilla module.  target_config is the
    target configuration.  opts are the command line options.
    """

    inform("Generating the C++ source for the Qsci module...")

    # Build the SIP command line.
    argv = [target_config.sip]

    argv.extend(sip_flags(target_config))

    if opts.no_timestamp:
        argv.append('-T')

    if not opts.no_docstrings:
        argv.append('-o');

    if target_config.prot_is_public:
        argv.append('-P');

    if opts.concat:
        argv.append('-j')
        argv.append(str(opts.split))

    if opts.tracing:
        argv.append('-r')

    argv.append('-c')
    argv.append('.')

    if opts.pyqt_package == 'PyQt5':
        argv.append('sip/qscimod5.sip')
    else:
        argv.append('sip/qscimod4.sip')

    rc = os.spawnv(os.P_WAIT, target_config.sip, argv)
    if rc != 0:
        error("%s returned exit code %d." % (target_config.sip, rc))

    # Generate the .pro file.
    generate_pro(target_config, opts)

    # Generate the Makefile.
    inform("Creating the Makefile for the Qsci module...")

    qmake_args = ['qmake']
    if target_config.qmake_spec != '':
        qmake_args.append('-spec')
        qmake_args.append(target_config.qmake_spec)
    qmake_args.append('Qsci.pro')

    rc = os.spawnv(os.P_WAIT, target_config.qmake, qmake_args)
    if rc != 0:
        error("%s returned exit code %d." % (target_config.qmake, rc))


def generate_pro(target_config, opts):
    """ Generate the .pro file for the QScintilla module.  target_config is the
    target configuration.  opts are the command line options.
    """

    # Without the 'no_check_exist' magic the target.files must exist when qmake
    # is run otherwise the install and uninstall targets are not generated.
    inform("Generating the .pro file for the Qsci module...")

    pro = open('Qsci.pro', 'w')

    pro.write('TEMPLATE = lib\n')
    pro.write('CONFIG += %s\n' % ('debug' if opts.debug else 'release'))
    pro.write('CONFIG += %s\n' % ('staticlib' if opts.static else 'plugin'))

    pro.write('''
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets printsupport
}
''')

    if not opts.static:
        # I don't really understand why the linker needs to find the Python
        # .lib file.
        pro.write('''
win32 {
    PY_MODULE = Qsci.pyd
    target.files = Qsci.pyd
    LIBS += -L%s
} else {
    PY_MODULE = Qsci.so
    target.files = Qsci.so
}

target.CONFIG = no_check_exist
''' % target_config.py_lib_dir)

    pro.write('''
target.path = %s
INSTALLS += target
''' % target_config.module_dir)

    pro.write('''
api.path = %s/api/python
api.files = QScintilla2.api
INSTALLS += api
''' % target_config.api_dir)

    pro.write('''
sip.path = %s/Qsci
sip.files =''' % target_config.qsci_sip_dir)
    for s in glob.glob('sip/*.sip'):
        pro.write(' \\\n    %s' % s)
    pro.write('''
INSTALLS += sip
''')

    pro.write('\n')

    # These optimisations could apply to other platforms.
    if target_config.py_platform == 'darwin':
        pro.write('QMAKE_CXXFLAGS += -fno-exceptions\n')

    if target_config.py_platform.startswith('linux'):
        pro.write('QMAKE_CXXFLAGS += -fno-exceptions\n')

        if not opts.static:
            if target_config.py_version >= 0x030000:
                entry_point = 'PyInit_Qsci'
            else:
                entry_point = 'initQsci'

            exp = open('Qsci.exp', 'wt')
            exp.write('{ global: %s; local: *; };' % entry_point)
            exp.close()

            pro.write('QMAKE_LFLAGS += -Wl,--version-script=Qsci.exp\n')

    if target_config.prot_is_public:
        pro.write('DEFINES += SIP_PROTECTED_IS_PUBLIC protected=public\n')

    if target_config.qscintilla_is_dll:
        pro.write('DEFINES += QSCINTILLA_DLL\n')

    pro.write('INCLUDEPATH += %s\n' % target_config.qsci_inc_dir)
    pro.write('INCLUDEPATH += %s\n' % target_config.py_inc_dir)
    if target_config.py_inc_dir != target_config.sip_inc_dir:
        pro.write('INCLUDEPATH += %s\n' % target_config.sip_inc_dir)

    pro.write('LIBS += -L%s -lqscintilla2\n' % target_config.qsci_lib_dir)

    if not opts.static:
        pro.write('''
win32 {
    QMAKE_POST_LINK = $(COPY_FILE) $(DESTDIR_TARGET) $$PY_MODULE
} else {
    QMAKE_POST_LINK = $(COPY_FILE) $(TARGET) $$PY_MODULE
}

macx {
    QMAKE_LFLAGS += "-undefined dynamic_lookup"
    QMAKE_POST_LINK = $$QMAKE_POST_LINK$$escape_expand(\\\\n\\\\t)$$quote(install_name_tool -change libqscintilla2.%s.dylib %s/libqscintilla2.%s.dylib $$PY_MODULE)
}
''' % (QSCI_API_MAJOR, target_config.qsci_lib_dir, QSCI_API_MAJOR))

    pro.write('\n')
    pro.write('TARGET = Qsci\n')
    pro.write('HEADERS = sipAPIQsci.h\n')

    pro.write('SOURCES =')
    for s in glob.glob('*.cpp'):
        pro.write(' \\\n    %s' % s)
    pro.write('\n')

    pro.close()


def check_sip(target_config):
    """ Check that the version of sip is good enough.  target_config is the
    target configuration.
    """

    if target_config.sip is None:
        error(
                "Make sure you have a working sip on your PATH or use the "
                "--sip argument to explicitly specify a working sip.")

    pipe = os.popen(' '.join([target_config.sip, '-V']))

    for l in pipe:
        version_str = l.strip()
        break
    else:
        error("'%s -V' did not generate any output." % target_config.sip)

    pipe.close()

    if 'snapshot' not in version_str:
        version = version_from_string(version_str)
        if version is None:
            error(
                    "'%s -V' generated unexpected output: '%s'." % (
                            target_config.sip, version_str))

        min_version = version_from_string(SIP_MIN_VERSION)
        if version < min_version:
            error(
                    "This version of QScintilla requires sip %s or later." %
                            SIP_MIN_VERSION)

    inform("sip %s is being used." % version_str)


def version_from_string(version_str):
    """ Convert a version string of the form m.n or m.n.o to an encoded version
    number (or None if it was an invalid format).  version_str is the version
    string.
    """

    parts = version_str.split('.')
    if not isinstance(parts, list):
        return None

    if len(parts) == 2:
        parts.append('0')

    if len(parts) != 3:
        return None

    version = 0

    for part in parts:
        try:
            v = int(part)
        except ValueError:
            return None

        version = (version << 8) + v

    return version


def main(argv):
    """ Create the configuration module module.  argv is the list of command
    line arguments.
    """

    # Create the default target configuration.
    target_config = TargetConfiguration()

    # Parse the command line.
    p = create_optparser(target_config)
    opts, args = p.parse_args()

    if args:
        p.print_help()
        sys.exit(2)

    # Query qmake for the basic configuration information.
    target_config.get_qt_configuration(opts)

    # Update the target configuration.
    if opts.config_file is not None:
        target_config.from_configuration_file(opts.config_file)
    else:
        target_config.from_introspection(opts.pyqt_package)

    target_config.override_defaults(opts)

    # Check SIP is new enough.
    check_sip(target_config)

    # Check for QScintilla.
    check_qscintilla(target_config)

    # Tell the user what's been found.
    inform_user(target_config)

    # Generate the code.
    generate_code(target_config, opts)


###############################################################################
# The script starts here.
###############################################################################

if __name__ == '__main__':
    try:
        main(sys.argv)
    except SystemExit:
        raise
    except:
        sys.stderr.write(
"""An internal error occured.  Please report all the output from the program,
including the following traceback, to support@riverbankcomputing.com.
""")
        raise
