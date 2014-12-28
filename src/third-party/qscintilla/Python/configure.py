# Copyright (c) 2014, Riverbank Computing Limited
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# This is v1.2 of this boilerplate.


import sys
import os
import glob
import optparse

try:
    import sysconfig
except ImportError:
    from distutils import sysconfig


###############################################################################
# You shouldn't need to modify anything above this line.
###############################################################################


# This must be kept in sync with Python/configure-old.py, qscintilla.pro,
# example-Qt4Qt5/application.pro and designer-Qt4Qt5/designer.pro.
QSCI_API_MAJOR = 11


class ModuleConfiguration(object):
    """ This class encapsulates all the module specific information needed by
    the rest of this script to implement a configure.py script for modules that
    build on top of PyQt.  Functions implemented by the rest of this script
    that begin with an underscore are considered internal and shouldn't be
    called from here.
    """

    # The name of the module as it would be used in an import statement.
    name = 'Qsci'

    # The descriptive name of the module.  This is used in help text and error
    # messages.
    descriptive_name = "QScintilla"

    # The version of the module as a string.  Set it to None if you don't
    # provide version information.
    version = '2.8.4'

    # Set if a configuration script is provided that handles versions of PyQt4
    # prior to v4.10 (i.e. versions where the pyqtconfig.py module is
    # available).  If provided the script must be called configure-old.py and
    # be in the same directory as this script.
    legacy_configuration_script = True

    # The minimum version of SIP that is required.  This should be a
    # dot-separated string of two or three integers (e.g. '1.0', '4.10.3').  If
    # it is None or an empty string then the version is not checked.
    minimum_sip_version = '4.16'

    # Set if support for C++ exceptions can be disabled.
    no_exceptions = True

    # Set if the module supports redefining 'protected' as 'public'.
    protected_is_public_is_supported = True

    # Set if the module supports PyQt4.
    pyqt4_is_supported = True

    # Set if the module supports PyQt5.
    pyqt5_is_supported = True

    # Set if the PyQt5 support is the default.  It is ignored unless both
    # 'pyqt4_is_supported' and 'pyqt5_is_supported' are set.
    pyqt5_is_default = False

    # The name (without the .api extension) of the name of the QScintilla API
    # file to be generated.  If it is None or an empty string then an API file
    # is not generated.
    qscintilla_api_file = 'QScintilla2'

    # The email address that will be included when an error in the script is
    # detected.  Leave it blank if you don't want to include an address.
    support_email_address = 'support@riverbankcomputing.com'

    # Set if the user can provide a configuration file.  It is normally only
    # used if cross-compilation is supported.
    user_configuration_file_is_supported = True

    # Set if the user is allowed to pass PyQt sip flags on the command line.
    # It is normally only used if cross-compilation is supported.  It is
    # ignored unless at least one of 'pyqt4_is_supported' or
    # 'pyqt5_is_supported' is set.
    user_pyqt_sip_flags_is_supported = True

    def init_target_configuration(self, target_configuration):
        """ Perform any module specific initialisation of the target
        target configuration.  Typically this is the initialisation of module
        specific attributes.  To avoid name clashes attributes should be given
        a module specific prefix.  target_configuration is the target
        configuration.
        """

        target_configuration.qsci_version = None
        target_configuration.qsci_inc_dir = None
        target_configuration.qsci_lib_dir = None
        target_configuration.qsci_is_dll = (target_configuration.py_platform == 'win32')
        target_configuration.qsci_sip_dir = None

    def init_optparser(self, optparser, target_configuration):
        """ Perform any module specific initialisation of the command line
        option parser.  To avoid name clashes destination attributes should be
        given a module specific prefix.  optparser is the option parser.
        target_configuration is the target configuration.
        """

        optparser.add_option('--qsci-incdir', '-n', dest='qsci_inc_dir',
                type='string', default=None, action='callback',
                callback=optparser_store_abspath_dir, metavar="DIR",
                help="the directory containing the QScintilla Qsci header "
                        "file directory is DIR [default: QT_INSTALL_HEADERS]")

        optparser.add_option('--qsci-libdir', '-o', dest='qsci_lib_dir',
                type='string', default=None, action='callback',
                callback=optparser_store_abspath_dir, metavar="DIR",
                help="the directory containing the QScintilla library is DIR "
                        "[default: QT_INSTALL_LIBS]")

        optparser.add_option('--no-dll', '-s', dest='qsci_is_dll',
                default=None, action='store_false',
                help="QScintilla is a static library and not a Windows DLL")

        optparser.add_option('--qsci-sipdir', '-v', dest='qsci_sip_dir',
                type='string', default=None, action='callback',
                callback=optparser_store_abspath_dir, metavar="DIR",
                help="the QScintilla .sip files will be installed in DIR "
                        "[default: %s]" % target_configuration.pyqt_sip_dir)

        optparser.add_option("--no-sip-files", action="store_true",
                default=False, dest="qsci_no_sip_files",
                help="disable the installation of the .sip files "
                        "[default: enabled]")

    def apply_options(self, target_configuration, options):
        """ Apply the module specific command line options to the target
        configuration.  target_configuration is the target configuration.
        options are the parsed options.
        """

        if options.qsci_inc_dir is not None:
            target_configuration.qsci_inc_dir = options.qsci_inc_dir

        if options.qsci_lib_dir is not None:
            target_configuration.qsci_lib_dir = options.qsci_lib_dir

        if options.qsci_is_dll is not None:
            target_configuration.qsci_is_dll = options.qsci_is_dll

        if options.qsci_sip_dir is not None:
            target_configuration.qsci_sip_dir = options.qsci_sip_dir
        else:
            target_configuration.qsci_sip_dir = target_configuration.pyqt_sip_dir

        if options.qsci_no_sip_files:
            target_configuration.qsci_sip_dir = ''

    def check_module(self, target_configuration):
        """ Perform any module specific checks now that the target
        configuration is complete.  target_configuration is the target
        configuration.
        """

        # Find the QScintilla header files.
        inc_dir = target_configuration.qsci_inc_dir
        if inc_dir is None:
            inc_dir = target_configuration.qt_inc_dir

        sciglobal = os.path.join(inc_dir, 'Qsci', 'qsciglobal.h')

        if not os.access(sciglobal, os.F_OK):
            error(
                    "Qsci/qsciglobal.h could not be found in %s. If "
                    "QScintilla is installed then use the --qsci-incdir "
                    "argument to explicitly specify the correct "
                    "directory." % inc_dir)

        # Get the QScintilla version string.
        qsci_version = read_define(sciglobal, 'QSCINTILLA_VERSION_STR')
        if qsci_version is None:
            error(
                    "The QScintilla version number could not be determined by "
                    "reading %s." % sciglobal)

        lib_dir = target_configuration.qsci_lib_dir
        if lib_dir is None:
            lib_dir = target_configuration.qt_lib_dir

        if not glob.glob(os.path.join(lib_dir, '*qscintilla2*')):
            error(
                    "The QScintilla library could not be found in %s. If "
                    "QScintilla is installed then use the --qsci-libdir "
                    "argument to explicitly specify the correct "
                    "directory." % lib_dir)

        # Because we include the Python bindings with the C++ code we can
        # reasonably force the same version to be used and not bother about
        # versioning in the .sip files.
        if qsci_version != self.version:
            error(
                    "QScintilla %s is being used but the Python bindings %s "
                    "are being built. Please use matching "
                    "versions." % (qsci_version, self.version))

        target_configuration.qsci_version = qsci_version

    def inform_user(self, target_configuration):
        """ Inform the user about module specific configuration information.
        target_configuration is the target configuration.
        """

        inform("QScintilla %s is being used." %
                target_configuration.qsci_version)

        if target_configuration.qsci_sip_dir != '':
            inform("The QScintilla .sip files will be installed in %s." %
                    target_configuration.qsci_sip_dir)

    def pre_code_generation(self, target_config):
        """ Perform any module specific initialisation prior to generating the
        code.  target_config is the target configuration.
        """

        # Nothing to do.

    def get_sip_flags(self, target_configuration):
        """ Return the list of module-specific flags to pass to SIP.
        target_configuration is the target configuration.
        """

        # Nothing to do.
        return []

    def get_sip_file(self, target_configuration):
        """ Return the name of the module's .sip file.  target_configuration is
        the target configuration.
        """

        return 'sip/qscimod5.sip' if target_configuration.pyqt_package == 'PyQt5' else 'sip/qscimod4.sip'

    def get_sip_installs(self, target_configuration):
        """ Return a tuple of the installation directory of the module's .sip
        files and a sequence of the names of each of the .sip files relative to
        the directory containing this configuration script.  None is returned
        if the module's .sip files are not to be installed.
        target_configuration is the target configuration.
        """

        if target_configuration.qsci_sip_dir == '':
            return None

        path = os.path.join(target_configuration.qsci_sip_dir, 'Qsci')
        files = glob.glob('sip/*.sip')

        return path, files

    def get_qmake_configuration(self, target_configuration):
        """ Return a dict of qmake configuration values for CONFIG, DEFINES,
        INCLUDEPATH, LIBS and QT.  If value names (i.e. dict keys) have either
        'Qt4' or 'Qt5' prefixes then they are specific to the corresponding
        version of Qt.  target_configuration is the target configuration.
        """

        qmake = {'CONFIG': 'qscintilla2'}

        if target_configuration.qsci_inc_dir is not None:
            qmake['INCLUDEPATH'] = quote(target_configuration.qsci_inc_dir)

        if target_configuration.qsci_lib_dir is not None:
            qmake['LIBS'] = '-L%s' % quote(target_configuration.qsci_lib_dir)

        if target_configuration.qsci_is_dll:
            qmake['DEFINES'] = 'QSCINTILLA_DLL'

        return qmake

    def get_mac_wrapped_library_file(self, target_configuration):
        """ Return the full pathname of the file that implements the library
        being wrapped by the module as it would be called on OS/X so that the
        module will reference it explicitly without DYLD_LIBRARY_PATH being
        set.  If it is None or an empty string then the default is used.
        target_configuration is the target configuration.
        """

        lib_dir = target_configuration.qsci_lib_dir
        if lib_dir is None:
            lib_dir = target_configuration.qt_lib_dir

        return os.path.join(lib_dir,
                'libqscintilla2.%s.dylib' % QSCI_API_MAJOR)


###############################################################################
# You shouldn't need to modify anything below this line.
###############################################################################


def error(msg):
    """ Display an error message and terminate.  msg is the text of the error
    message.
    """

    sys.stderr.write(_format("Error: " + msg) + "\n")
    sys.exit(1)


def inform(msg):
    """ Display an information message.  msg is the text of the error message.
    """

    sys.stdout.write(_format(msg) + "\n")


def quote(path):
    """ Return a path with quotes added if it contains spaces.  path is the
    path.
    """

    if ' ' in path:
        path = '"%s"' % path

    return path


def optparser_store_abspath(option, opt_str, value, parser):
    """ An optparser callback that saves an option as an absolute pathname. """

    setattr(parser.values, option.dest, os.path.abspath(value))


def optparser_store_abspath_dir(option, opt_str, value, parser):
    """ An optparser callback that saves an option as the absolute pathname
    of an existing directory.
    """

    if not os.path.isdir(value):
        raise optparse.OptionValueError("'%s' is not a directory" % value)

    setattr(parser.values, option.dest, os.path.abspath(value))


def optparser_store_abspath_exe(option, opt_str, value, parser):
    """ An optparser callback that saves an option as the absolute pathname
    of an existing executable.
    """

    if not os.access(value, os.X_OK):
        raise optparse.OptionValueError("'%s' is not an executable" % value)

    setattr(parser.values, option.dest, os.path.abspath(value))


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


def version_from_string(version_str):
    """ Convert a version string of the form m, m.n or m.n.o to an encoded
    version number (or None if it was an invalid format).  version_str is the
    version string.
    """

    parts = version_str.split('.')
    if not isinstance(parts, list):
        return None

    if len(parts) == 1:
        parts.append('0')

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


def _format(msg, left_margin=0, right_margin=78):
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


class _ConfigurationFileParser:
    """ A parser for configuration files. """

    def __init__(self, config_file):
        """ Read and parse a configuration file. """

        self._config = {}
        self._extrapolating = []

        cfg = open(config_file)
        line_nr = 0
        last_name = None

        section = ''
        section_config = {}
        self._config[section] = section_config

        for l in cfg:
            line_nr += 1

            # Strip comments.
            l = l.split('#')[0]

            # See if this might be part of a multi-line.
            multiline = (last_name is not None and len(l) != 0 and l[0] == ' ')

            l = l.strip()

            if l == '':
                last_name = None
                continue

            # See if this is a new section.
            if l[0] == '[' and l[-1] == ']':
                section = l[1:-1].strip()
                if section == '':
                    error(
                            "%s:%d: Empty section name." % (
                                    config_file, line_nr))

                if section in self._config:
                    error(
                            "%s:%d: Section '%s' defined more than once." % (
                                    config_file, line_nr, section))

                section_config = {}
                self._config[section] = section_config

                last_name = None
                continue

            parts = l.split('=', 1)
            if len(parts) == 2:
                name = parts[0].strip()
                value = parts[1].strip()
            elif multiline:
                name = last_name
                value = section_config[last_name]
                value += ' ' + l
            else:
                name = value = ''

            if name == '' or value == '':
                error("%s:%d: Invalid line." % (config_file, line_nr))

            section_config[name] = value
            last_name = name

        cfg.close()

    def sections(self):
        """ Return the list of sections, excluding the default one. """

        return [s for s in self._config.keys() if s != '']

    def preset(self, name, value):
        """ Add a preset value to the configuration. """

        self._config[''][name] = value

    def get(self, section, name, default=None):
        """ Get a configuration value while extrapolating. """

        # Get the name from the section, or the default section.
        value = self._config[section].get(name)
        if value is None:
            value = self._config[''].get(name)
            if value is None:
                if default is None:
                    error(
                            "Configuration file references non-existent name "
                            "'%s'." % name)

                return default

        # Handle any extrapolations.
        parts = value.split('%(', 1)
        while len(parts) == 2:
            prefix, tail = parts

            parts = tail.split(')', 1)
            if len(parts) != 2:
                error(
                        "Configuration file contains unterminated "
                        "extrapolated name '%s'." % tail)

            xtra_name, suffix = parts

            if xtra_name in self._extrapolating:
                error(
                        "Configuration file contains a recursive reference to "
                        "'%s'." % xtra_name)

            self._extrapolating.append(xtra_name)
            xtra_value = self.get(section, xtra_name)
            self._extrapolating.pop()

            value = prefix + xtra_value + suffix

            parts = value.split('%(', 1)

        return value

    def getboolean(self, section, name, default):
        """ Get a boolean configuration value while extrapolating. """

        value = self.get(section, name, default)

        # In case the default was returned.
        if isinstance(value, bool):
            return value

        if value in ('True', 'true', '1'):
            return True

        if value in ('False', 'false', '0'):
            return False

        error(
                "Configuration file contains invalid boolean value for "
                "'%s'." % name)

    def getlist(self, section, name, default):
        """ Get a list configuration value while extrapolating. """

        value = self.get(section, name, default)

        # In case the default was returned.
        if isinstance(value, list):
            return value

        return value.split()


class _HostPythonConfiguration:
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


class _TargetQtConfiguration:
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


class _TargetConfiguration:
    """ A container for the target configuration. """

    def __init__(self, module_config):
        """ Initialise the configuration with default values.  module_config is
        the module configuration.
        """

        # Values based on the host Python configuration.
        py_config = _HostPythonConfiguration()
        self.py_platform = py_config.platform
        self.py_version = py_config.version
        self.py_module_dir = py_config.module_dir
        self.py_inc_dir = py_config.inc_dir
        self.py_pylib_dir = py_config.lib_dir
        self.py_sip_dir = os.path.join(py_config.data_dir, 'sip')
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
        self.pyqt_sip_flags = None
        self.pyqt_version_str = ''
        self.qmake = self._find_exe('qmake')
        self.qt_version_str = ''
        self.sip = self._find_exe('sip')
        self.sip_version = None
        self.sysroot = ''

        self.prot_is_public = (self.py_platform.startswith('linux') or self.py_platform == 'darwin')

        if module_config.pyqt5_is_supported and module_config.pyqt4_is_supported:
            pyqt = 'PyQt5' if module_config.pyqt5_is_default else 'PyQt4'
        elif module_config.pyqt5_is_supported and not module_config.pyqt4_is_supported:
            pyqt = 'PyQt5'
        elif not module_config.pyqt5_is_supported and module_config.pyqt4_is_supported:
            pyqt = 'PyQt4'
        else:
            pyqt = None

        if pyqt is not None:
            self.module_dir = os.path.join(py_config.module_dir, pyqt)
            self.pyqt_sip_dir = os.path.join(self.py_sip_dir, pyqt)
        else:
            self.module_dir = py_config.module_dir
            self.pyqt_sip_dir = None

        self.pyqt_package = pyqt

        module_config.init_target_configuration(self)

    def update_from_configuration_file(self, config_file):
        """ Update the configuration with values from a file.  config_file
        is the name of the configuration file.
        """

        inform("Reading configuration from %s..." % config_file)

        parser = _ConfigurationFileParser(config_file)

        # Populate some presets from the command line.
        parser.preset('py_major', str(self.py_version >> 16))
        parser.preset('py_minor', str((self.py_version >> 8) & 0xff))
        parser.preset('sysroot', self.sysroot)

        if self.pyqt_package is None:
            section = ''
        else:
            # At the moment we only need to distinguish between PyQt4 and
            # PyQt5.  If that changes we may need a --target-pyqt-version
            # command line option.
            pyqt_version = 0x050000 if self.pyqt_package == 'PyQt5' else 0x040000

            # Find the section corresponding to the version of PyQt.
            section = None
            latest_section = -1

            for name in parser.sections():
                parts = name.split()
                if len(parts) != 2 or parts[0] != 'PyQt':
                    continue

                section_pyqt_version = version_from_string(parts[1])
                if section_pyqt_version is None:
                    continue

                # Major versions must match.
                if section_pyqt_version >> 16 != pyqt_version >> 16:
                    continue

                # It must be no later that the version of PyQt.
                if section_pyqt_version > pyqt_version:
                    continue

                # Save it if it is the latest so far.
                if section_pyqt_version > latest_section:
                    section = name
                    latest_section = section_pyqt_version

            if section is None:
                error(
                        "%s does not define a section that covers PyQt "
                        "v%s." % (config_file, self.pyqt_version_str))

        self.py_platform = parser.get(section, 'py_platform', self.py_platform)
        self.py_inc_dir = parser.get(section, 'py_inc_dir', self.py_inc_dir)
        self.py_pylib_dir = parser.get(section, 'py_pylib_dir',
                self.py_pylib_dir)

        self.sip_inc_dir = self.py_inc_dir

        self.module_dir = parser.get(section, 'module_dir', self.module_dir)

        if self.pyqt_package is not None:
            self.py_sip_dir = parser.get(section, 'py_sip_dir',
                    self.py_sip_dir)

            # Construct the SIP flags.
            flags = []

            flags.append('-t')
            flags.append(self._get_platform_tag())

            qt_version = version_from_string(self.qt_version_str)
            if qt_version is None:
                error("Unable to determine the version of Qt.")

            if self.pyqt_package == 'PyQt5':
                if qt_version < 0x050000:
                    error("PyQt5 requires Qt v5.0 or later.")

                if qt_version > 0x060000:
                    qt_version = 0x060000

                backstop = 'Qt_6_0_0'
            else:
                if qt_version > 0x050000:
                    qt_version = 0x050000

                backstop = 'Qt_5_0_0'

            major = (qt_version >> 16) & 0xff
            minor = (qt_version >> 8) & 0xff
            patch = qt_version & 0xff

            flags.append('-t')
            flags.append('Qt_%d_%d_%d' % (major, minor, patch))

            flags.append('-B')
            flags.append(backstop)

            for feat in parser.getlist(section, 'pyqt_disabled_features', []):
                flags.append('-x')
                flags.append(feat)

            self.pyqt_sip_flags = ' '.join(flags)

    def _get_platform_tag(self):
        """ Return the tag for the target platform. """

        # This replicates the logic in PyQt's configure scripts.
        if self.py_platform == 'win32':
            plattag = 'WS_WIN'
        elif self.py_platform == 'darwin':
            plattag = 'WS_MACX'
        else:
            plattag = 'WS_X11'

        return plattag

    def introspect_pyqt(self, module_config):
        """ Introspect PyQt to determine the sip flags required.  module_config
        is the module configuration.
        """

        if self.pyqt_package == 'PyQt5':
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

        self.pyqt_version_str = QtCore.PYQT_VERSION_STR
        self.qt_version_str = QtCore.qVersion()

        # See if we have a PyQt that embeds its configuration.
        try:
            pyqt_config = QtCore.PYQT_CONFIGURATION
        except AttributeError:
            pyqt_config = None

        if pyqt_config is None:
            if module_config.legacy_configuration_script:
                # Fallback to the old configuration script.
                config_script = sys.argv[0].replace('configure', 'configure-old')
                args = [sys.executable, config_script] + sys.argv[1:]

                try:
                    os.execv(sys.executable, args)
                except OSError:
                    pass

                error("Unable to execute '%s'" % config_script)

            error("PyQt v4.10 or later is required.")

        self.pyqt_sip_flags = pyqt_config['sip_flags']

    def apply_sysroot(self):
        """ Apply sysroot where necessary. """

        if self.sysroot != '':
            self.py_inc_dir = self._apply_sysroot(self.py_inc_dir)
            self.py_pylib_dir = self._apply_sysroot(self.py_pylib_dir)
            self.py_sip_dir = self._apply_sysroot(self.py_sip_dir)
            self.module_dir = self._apply_sysroot(self.module_dir)
            self.sip_inc_dir = self._apply_sysroot(self.sip_inc_dir)

    def _apply_sysroot(self, dir_name):
        """ Replace any leading sys.prefix of a directory name with sysroot.
        """

        if dir_name.startswith(sys.prefix):
            dir_name = self.sysroot + dir_name[len(sys.prefix):]

        return dir_name

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
        qt_config = _TargetQtConfiguration(self.qmake)

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

        self.qt_version_str = getattr(qt_config, 'QT_VERSION', '')
        self.api_dir = os.path.join(qt_config.QT_INSTALL_DATA, 'qsci')
        self.qt_inc_dir = qt_config.QT_INSTALL_HEADERS
        self.qt_lib_dir = qt_config.QT_INSTALL_LIBS

        if self.sysroot == '':
            self.sysroot = getattr(qt_config, 'QT_SYSROOT', '')

    def apply_pre_options(self, opts):
        """ Apply options from the command line that influence subsequent
        configuration.  opts are the command line options.
        """

        # Get the system root.
        if opts.sysroot is not None:
            self.sysroot = opts.sysroot

        if self.pyqt_package is not None:
            try:
                self.pyqt_package = opts.pyqt_package
            except AttributeError:
                # Multiple PyQt versions are not supported.
                pass

            self.module_dir = os.path.join(self.py_module_dir,
                    self.pyqt_package)

    def apply_post_options(self, opts, module_config):
        """ Apply options from the command line that override the previous
        configuration.  opts are the command line options.  module_config is
        the module configuration.
        """

        if self.pyqt_package is not None:
            if module_config.user_pyqt_sip_flags_is_supported:
                if opts.pyqt_sip_flags is not None:
                    self.pyqt_sip_flags = opts.pyqt_sip_flags

            if opts.pyqt_sip_dir is not None:
                self.pyqt_sip_dir = opts.pyqt_sip_dir
            else:
                self.pyqt_sip_dir = os.path.join(self.py_sip_dir,
                        self.pyqt_package)

        if module_config.qscintilla_api_file:
            if opts.apidir is not None:
                self.api_dir = opts.apidir

            if opts.no_qsci_api:
                self.api_dir = ''

        if opts.destdir is not None:
            self.module_dir = opts.destdir

        if opts.qmakespec is not None:
            self.qmake_spec = opts.qmakespec

        if module_config.protected_is_public_is_supported:
            if opts.prot_is_public is not None:
                self.prot_is_public = opts.prot_is_public
        else:
            self.prot_is_public = False

        if opts.sip_inc_dir is not None:
            self.sip_inc_dir = opts.sip_inc_dir

        if opts.sip is not None:
            self.sip = opts.sip

        module_config.apply_options(self, opts)

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


def _create_optparser(target_config, module_config):
    """ Create the parser for the command line.  target_config is the target
    configuration containing default values.  module_config is the module
    configuration.
    """

    module_name = module_config.descriptive_name

    p = optparse.OptionParser(usage="python %prog [options]",
            version=module_config.version)

    p.add_option('--spec', dest='qmakespec', default=None, action='store',
            metavar="SPEC",
            help="pass -spec SPEC to qmake [default: %s]" % "don't pass -spec" if target_config.qmake_spec == '' else target_config.qmake_spec)

    if module_config.qscintilla_api_file:
        p.add_option('--apidir', '-a', dest='apidir', type='string',
                default=None, action='callback',
                callback=optparser_store_abspath, metavar="DIR", 
                help="the QScintilla API file will be installed in DIR "
                        "[default: QT_INSTALL_DATA/qsci]")
        p.add_option('--no-qsci-api', dest='no_qsci_api', default=False,
                action='store_true',
                help="disable the installation of the QScintilla API file "
                        "[default: enabled]")

    if module_config.user_configuration_file_is_supported:
        p.add_option('--configuration', dest='config_file', type='string',
                default=None, action='callback',
                callback=optparser_store_abspath, metavar="FILE",
                help="FILE defines the target configuration")

    p.add_option('--destdir', '-d', dest='destdir', type='string',
            default=None, action='callback', callback=optparser_store_abspath,
            metavar="DIR",
            help="install the %s module in DIR [default: %s]" %
                    (module_name, target_config.module_dir))

    if module_config.protected_is_public_is_supported:
        p.add_option('--protected-is-public', dest='prot_is_public',
                default=None, action='store_true',
                help="enable building with 'protected' redefined as 'public' "
                        "[default: %s]" % target_config.prot_is_public)
        p.add_option('--protected-not-public', dest='prot_is_public',
                action='store_false',
                help="disable building with 'protected' redefined as 'public'")

    if target_config.pyqt_package is not None:
        pyqt = target_config.pyqt_package

        if module_config.pyqt5_is_supported and module_config.pyqt4_is_supported:
            p.add_option('--pyqt', dest='pyqt_package', type='choice',
                    choices=['PyQt4', 'PyQt5'], default=pyqt,
                    action='store', metavar="PyQtn",
                    help="configure for PyQt4 or PyQt5 [default: %s]" % pyqt)

        if module_config.user_pyqt_sip_flags_is_supported:
            p.add_option('--pyqt-sip-flags', dest='pyqt_sip_flags',
                default=None, action='store', metavar="FLAGS",
                help="the sip flags used to build PyQt [default: query PyQt]")

    if sys.platform != 'win32':
        p.add_option('--qmake', '-q', dest='qmake', type='string',
                default=None, action='callback',
                callback=optparser_store_abspath_exe, metavar="FILE",
                help="the pathname of qmake is FILE [default: "
                        "%s]" % (target_config.qmake or "None"))

    p.add_option('--sip', dest='sip', type='string', default=None,
            action='callback', callback=optparser_store_abspath_exe,
            metavar="FILE",
            help="the pathname of sip is FILE [default: "
                    "%s]" % (target_config.sip or "None"))
    p.add_option('--sip-incdir', dest='sip_inc_dir', type='string',
            default=None, action='callback',
            callback=optparser_store_abspath_dir, metavar="DIR",
            help="the directory containing the sip.h header file file is DIR "
                    "[default: %s]" % target_config.sip_inc_dir)

    if target_config.pyqt_package is not None:
        p.add_option('--pyqt-sipdir', dest='pyqt_sip_dir', type='string',
                default=None, action='callback',
                callback=optparser_store_abspath_dir, metavar="DIR",
                help="the directory containing the PyQt .sip files is DIR "
                        "[default: %s]" % target_config.pyqt_sip_dir)

    p.add_option('--concatenate', '-c', dest='concat', default=False,
            action='store_true', 
            help="concatenate the C++ source files")
    p.add_option('--concatenate-split', '-j', dest='split', type='int',
            default=1, metavar="N",
            help="split the concatenated C++ source files into N pieces "
                    "[default: 1]")
    p.add_option('--static', '-k', dest='static', default=False,
            action='store_true',
            help="build the %s module as a static library" % module_name)
    p.add_option("--sysroot", dest='sysroot', type='string', action='callback',
            callback=optparser_store_abspath_dir, metavar="DIR",
            help="DIR is the target system root directory")
    p.add_option('--no-docstrings', dest='no_docstrings', default=False,
            action='store_true',
            help="disable the generation of docstrings")
    p.add_option('--trace', '-r', dest='tracing', default=False,
            action='store_true',
            help="build the %s module with tracing enabled" % module_name)
    p.add_option('--debug', '-u', default=False, action='store_true',
            help="build the %s module with debugging symbols" % module_name)
    p.add_option('--verbose', '-w', dest='verbose', default=False,
            action='store_true',
            help="enable verbose output during configuration")
    p.add_option('--no-timestamp', '-T', dest='no_timestamp', default=False,
            action='store_true',
            help="suppress timestamps in the header comments of generated "
                    "code [default: include timestamps]")

    module_config.init_optparser(p, target_config)

    return p


def _inform_user(target_config, module_config):
    """ Tell the user the values that are going to be used.  target_config is
    the target configuration.  module_config is the module configuration.
    """

    module_name = module_config.descriptive_name

    inform("Configuring %s %s..." % (module_name, module_config.version))

    module_config.inform_user(target_config)

    inform("The %s module will be installed in %s." %
            (module_name, target_config.module_dir))

    if target_config.pyqt_version_str != '':
        inform("PyQt %s is being used." % target_config.pyqt_version_str)
    else:
        inform("%s is being used." % target_config.pyqt_package)

    if target_config.qt_version_str != '':
        inform("Qt %s is being used." % target_config.qt_version_str)

    if target_config.sysroot != '':
        inform("The system root directory is %s." % target_config.sysroot)

    inform("sip %s is being used." % target_config.sip_version)
    inform("The sip executable is %s." % target_config.sip)

    if target_config.prot_is_public:
        inform(
                "The %s module is being built with 'protected' redefined as "
                "'public'." % module_name)

    if module_config.qscintilla_api_file and target_config.api_dir != '':
        inform("The QScintilla API file will be installed in %s." %
                os.path.join(target_config.api_dir, 'api', 'python'))


def _generate_code(target_config, opts, module_config):
    """ Generate the code for the QScintilla module.  target_config is the
    target configuration.  opts are the command line options.  module_config is
    the module configuration.
    """

    inform(
            "Generating the C++ source for the %s module..." %
                    module_config.name)

    # Build the SIP command line.
    argv = [quote(target_config.sip)]

    # Add the module-specific flags.
    argv.extend(module_config.get_sip_flags(target_config))

    if target_config.pyqt_package is not None:
        # Get the flags used for the main PyQt module.
        argv.extend(target_config.pyqt_sip_flags.split())

        # Add PyQt's .sip files to the search path.
        argv.append('-I')
        argv.append(target_config.pyqt_sip_dir)

    if module_config.qscintilla_api_file and target_config.api_dir != '':
        # Generate the API file.
        argv.append('-a')
        argv.append(module_config.qscintilla_api_file + '.api')

    if target_config.prot_is_public:
        argv.append('-P');

    if opts.no_timestamp:
        argv.append('-T')

    if not opts.no_docstrings:
        argv.append('-o');

    if opts.concat:
        argv.append('-j')
        argv.append(str(opts.split))

    if opts.tracing:
        argv.append('-r')

    argv.append('-c')
    argv.append('.')

    argv.append(module_config.get_sip_file(target_config))

    check_file = 'sipAPI%s.h' % module_config.name
    _remove_file(check_file)

    _run_command(' '.join(argv), opts.verbose)

    if not os.access(check_file, os.F_OK):
        error("Unable to create the C++ code.")

    # Generate the .pro file.
    pro = _generate_pro(target_config, opts, module_config)

    # Generate the Makefile.
    _run_qmake(target_config, opts.verbose, pro, module_config)


def _get_qt_qmake_config(qmake_config, qt_version):
    """ Return a dict of qmake configuration values for a specific Qt version.
    """

    qt_qmake_config = {}

    for name, value in qmake_config.items():
        name_parts = name.split(':')
        if len(name_parts) == 2 and name_parts[0] == qt_version:
            qt_qmake_config[name_parts[1]] = value

    return qt_qmake_config


def _write_qt_qmake_config(qt_qmake_config, pro):
    """ Write the qmake configuration values to a .pro file. """

    for name in ('QT', 'CONFIG', 'DEFINES', 'INCLUDEPATH', 'LIBS'):
        value = qt_qmake_config.get(name)
        if value:
            pro.write('    %s += %s\n' % (name, value))


def _generate_pro(target_config, opts, module_config):
    """ Generate the .pro file for the module and return its name.
    target_config is the target configuration.  opts are the command line
    options.  module_config is the module configuration.
    """

    inform("Generating the .pro file for the %s module..." % module_config.name)

    # Without the 'no_check_exist' magic the target.files must exist when qmake
    # is run otherwise the install and uninstall targets are not generated.

    qmake_config = module_config.get_qmake_configuration(target_config)

    pro_name = module_config.name + '.pro'

    pro = open(pro_name, 'w')

    pro.write('TEMPLATE = lib\n')

    qt = qmake_config.get('QT')
    if qt:
        pro.write('QT += %s\n' % qt)

    pro.write('CONFIG += %s\n' % ('debug' if opts.debug else 'release'))
    pro.write('CONFIG += %s\n' % ('staticlib' if opts.static else 'plugin'))

    config = qmake_config.get('CONFIG')
    if config:
        pro.write('CONFIG += %s\n' % config)

    # Work around QTBUG-39300.
    pro.write('CONFIG -= android_install\n')

    qt5_qmake_config = _get_qt_qmake_config(qmake_config, 'Qt5')
    qt4_qmake_config = _get_qt_qmake_config(qmake_config, 'Qt4')

    if qt5_qmake_config or qt4_qmake_config:
        pro.write('''
greaterThan(QT_MAJOR_VERSION, 4) {
''')

        if qt5_qmake_config:
            _write_qt_qmake_config(qt5_qmake_config, pro)

        if qt4_qmake_config:
            pro.write('} else {\n')
            _write_qt_qmake_config(qt4_qmake_config, pro)

        pro.write('}\n')

    mname = module_config.name

    if not opts.static:
        pro.write('''
win32 {
    PY_MODULE = %s.pyd
    target.files = %s.pyd
    LIBS += -L%s
} else {
    PY_MODULE = %s.so
    target.files = %s.so
}

target.CONFIG = no_check_exist
''' % (mname, mname, quote(target_config.py_pylib_dir), mname, mname))

    pro.write('''
target.path = %s
INSTALLS += target
''' % quote(target_config.module_dir))

    if module_config.qscintilla_api_file and target_config.api_dir != '':
        pro.write('''
api.path = %s/api/python
api.files = %s.api
INSTALLS += api
''' % (target_config.api_dir, module_config.qscintilla_api_file))

    sip_installs = module_config.get_sip_installs(target_config)
    if sip_installs is not None:
        path, files = sip_installs

        pro.write('''
sip.path = %s
sip.files =''' % quote(path))

        for f in files:
            pro.write(' \\\n    %s' % f)

        pro.write('''
INSTALLS += sip
''')

    pro.write('\n')

    # These optimisations could apply to other platforms.
    if module_config.no_exceptions:
        if target_config.py_platform.startswith('linux') or target_config.py_platform == 'darwin':
            pro.write('QMAKE_CXXFLAGS += -fno-exceptions\n')

    if target_config.py_platform.startswith('linux') and not opts.static:
        if target_config.py_version >= 0x030000:
            entry_point = 'PyInit_%s' % mname
        else:
            entry_point = 'init%s' % mname

        exp = open('%s.exp' % mname, 'wt')
        exp.write('{ global: %s; local: *; };' % entry_point)
        exp.close()

        pro.write('QMAKE_LFLAGS += -Wl,--version-script=%s.exp\n' % mname)

    if target_config.prot_is_public:
        pro.write('DEFINES += SIP_PROTECTED_IS_PUBLIC protected=public\n')

    defines = qmake_config.get('DEFINES')
    if defines:
        pro.write('DEFINES += %s\n' % defines)

    includepath = qmake_config.get('INCLUDEPATH')
    if includepath:
        pro.write('INCLUDEPATH += %s\n' % includepath)

    # Make sure the SIP include directory is searched before the Python include
    # directory if they are different.
    pro.write('INCLUDEPATH += %s\n' % quote(target_config.sip_inc_dir))
    if target_config.py_inc_dir != target_config.sip_inc_dir:
        pro.write('INCLUDEPATH += %s\n' % quote(target_config.py_inc_dir))

    libs = qmake_config.get('LIBS')
    if libs:
        pro.write('LIBS += %s\n' % libs)

    if not opts.static:
        pro.write('''
win32 {
    QMAKE_POST_LINK = $(COPY_FILE) $(DESTDIR_TARGET) $$PY_MODULE
} else {
    QMAKE_POST_LINK = $(COPY_FILE) $(TARGET) $$PY_MODULE
}

macx {
    QMAKE_LFLAGS += "-undefined dynamic_lookup"
    greaterThan(QT_MAJOR_VERSION, 4) {
        QMAKE_LFLAGS += "-install_name $$absolute_path($$PY_MODULE, $$target.path)"
    }
''')

        dylib = module_config.get_mac_wrapped_library_file(target_config)

        if dylib:
            pro.write('''
    QMAKE_POST_LINK = $$QMAKE_POST_LINK$$escape_expand(\\\\n\\\\t)$$quote(install_name_tool -change %s %s $$PY_MODULE)
''' % (os.path.basename(dylib), dylib))

        pro.write('}\n')

    pro.write('\n')
    pro.write('TARGET = %s\n' % mname)
    pro.write('HEADERS = sipAPI%s.h\n' % mname)

    pro.write('SOURCES =')
    for s in glob.glob('*.cpp'):
        pro.write(' \\\n    %s' % s)
    pro.write('\n')

    pro.close()

    return pro_name


def _run_qmake(target_config, verbose, pro_name, module_config):
    """ Run qmake against a .pro file.  target_config is the target
    configuration.  verbose is set if the output is to be displayed.  pro_name
    is the name of the .pro file.  module_config is the module configuration.
    """

    inform("Creating the Makefile for the %s module..." % module_config.name)

    # qmake doesn't behave consistently if it is not run from the directory
    # containing the .pro file - so make sure it is.
    pro_dir, pro_file = os.path.split(pro_name)
    if pro_dir != '':
        cwd = os.getcwd()
        os.chdir(pro_dir)
    else:
        cwd = None

    mf = 'Makefile'

    _remove_file(mf)

    args = [quote(target_config.qmake)]

    if target_config.qmake_spec != '':
        args.append('-spec')
        args.append(target_config.qmake_spec)

    args.append(pro_file)

    _run_command(' '.join(args), verbose)

    if not os.access(mf, os.F_OK):
        error(
                "%s failed to create a Makefile from %s." %
                        (target_config.qmake, pro_name))

    # Restore the current directory.
    if cwd is not None:
        os.chdir(cwd)


def _run_command(cmd, verbose):
    """ Run a command and display the output if requested.  cmd is the command
    to run.  verbose is set if the output is to be displayed.
    """

    if verbose:
        sys.stdout.write(cmd + "\n")

    fout = _get_command_output(cmd)

    # Read stdout and stderr until there is no more output.
    lout = fout.readline()
    while lout:
        if verbose:
            if sys.hexversion >= 0x03000000:
                sys.stdout.write(str(lout, encoding=sys.stdout.encoding))
            else:
                sys.stdout.write(lout)

        lout = fout.readline()

    fout.close()

    try:
        os.wait()
    except:
        pass


def _get_command_output(cmd):
    """ Return a pipe from which a command's output can be read.  cmd is the
    command.
    """

    try:
        import subprocess
    except ImportError:
        _, sout = os.popen4(cmd)

        return sout

    p = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    return p.stdout


def _remove_file(fname):
    """ Remove a file which may or may not exist.  fname is the name of the
    file.
    """

    try:
        os.remove(fname)
    except OSError:
        pass


def _check_sip(target_config, module_config):
    """ Check that the version of sip is good enough.  target_config is the
    target configuration.  module_config is the module configuration.
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

    if 'snapshot' not in version_str and 'preview' not in version_str:
        version = version_from_string(version_str)
        if version is None:
            error(
                    "'%s -V' generated unexpected output: '%s'." % (
                            target_config.sip, version_str))

        min_sip_version = module_config.minimum_sip_version
        if min_sip_version:
            min_version = version_from_string(min_sip_version)
            if version < min_version:
                error(
                        "This version of %s requires sip %s or later." %
                                (module_config.descriptive_name,
                                        min_sip_version))

    target_config.sip_version = version_str


def _main(argv, module_config):
    """ Create the configuration module module.  argv is the list of command
    line arguments.  module_config is the module configuration.
    """

    # Create the default target configuration.
    target_config = _TargetConfiguration(module_config)

    # Parse the command line.
    p = _create_optparser(target_config, module_config)
    opts, args = p.parse_args()

    if args:
        p.print_help()
        sys.exit(2)

    target_config.apply_pre_options(opts)

    # Query qmake for the basic configuration information.
    target_config.get_qt_configuration(opts)

    # Update the target configuration.
    if module_config.user_configuration_file_is_supported:
        config_file = opts.config_file
    else:
        config_file = None

    if config_file is not None:
        target_config.update_from_configuration_file(config_file)
    else:
        target_config.apply_sysroot()

    target_config.apply_post_options(opts, module_config)

    if target_config.pyqt_package is not None:
        if target_config.pyqt_sip_flags is None:
            target_config.introspect_pyqt(module_config)

    # Check SIP is new enough.
    _check_sip(target_config, module_config)

    # Perform any module specific checks now that all other information has
    # been captured.
    module_config.check_module(target_config)

    # Tell the user what's been found.
    _inform_user(target_config, module_config)

    # Allow for module specific hacks.
    module_config.pre_code_generation(target_config)

    # Generate the code.
    _generate_code(target_config, opts, module_config)


###############################################################################
# The script starts here.
###############################################################################

if __name__ == '__main__':
    module_config = ModuleConfiguration()

    try:
        _main(sys.argv, module_config)
    except SystemExit:
        raise
    except:
        if module_config.support_email_address:
            sys.stderr.write(
"""An internal error occured.  Please report all the output from the program,
including the following traceback, to %s.
""" % module_config.support_email_address)

        raise
