/** @mainpage LibPkg
 * @section intro Introduction
 * LibPkg is a package management library for RISC OS.  It includes
 * classes to:
 * - represent version numbers, status records and control files;
 * - access the content of package files; and
 * - install and remove packages.
 *
 * The first use of LibPkg will be as the back-end of RiscPkg, but it
 * is not tied to any particular user interface and there could be other
 * front-ends in the future.  Parts of LibPkg are likely to be useful
 * for writing package development tools and other types of package
 * management utility.
 *
 * @section warning WARNING
 * LibPkg is currently at an early stage of development and it has not
 * yet received extensive testing.  Package managers by their nature
 * have the potential to do serious harm to your filesystem.  At this
 * time it would be unwise to use LibPkg on a system you do not consider
 * to be expendable.
 *
 * (If LibPkg does go wrong then it is most likely to corrupt software
 * that it itself has installed, closely followed by the contents of
 * the !Boot directory.  It is much less likely to touch user data,
 * however now would be a good time to review your backup policy.)
 * @section requirements Requirements
 * Supported operating systems include:
 * - RISC OS 4.02
 * - RISC OS 5.03
 *
 * Supported compilers include:
 * - GCC 4.7.4 (release 1)
 *
 * The Acorn C/C++ compiler (up to version 3.19 at least) is not
 * supported because there are important parts of the C++ language
 * that it does not implement.
 * @section installation Installation
 * The !LibPkg application directory can be located anywhere.
 *
 * Ensure that it has been booted and/or run before attempting to
 * compile or link against it.
 *
 * (If two or more versions are present, running !LibPkg will change
 * the active version whereas booting it will not.)
 * @section use Use
 * When compiling, append -Ilibpkg: to the GCC command line.  This
 * adds libpkg: to the list of paths that are searched for include
 * files.
 *
 * When linking, append -LLibPkg: -lpkg.a to the GCC command line.
 * This adds libpkg:libpkg.a to the list of libraries searched by the
 * linker.
 *
 * Include directives should be of the form:
 * @code
 * #include "libpkg/control.h"
 * @endcode
 * Be aware that all LibPkg classes are declared within the namespace
 * pkg.  You must either specify the namespace expicitly:
 * @code
 * pkg::control
 * @endcode
 * or place a using directive at the start of each source file:
 * @code
 * using namespace control;
 * @endcode
 *
 * Documentation available from the RiscPkg web site currently includes:
 * - the LibPkg reference manual
 *
 * @section building Building from Source
 * To build LibPkg from souce code the following tools must be present
 * on the Run$Path:
 * - make
 * - gcc
 * - libfile
 * - fixdeps
 *
 * Also required are the libraries:
 * - zlib1g
 * - libcurl3
 *
 * Suitable versions of make, gcc and libfile may be found within
 * supported distributions of GCC.  fixdeps can be downloaded from
 * www.riscpkg.org or using !RiscPkg or !PackMan.
 *
 * No configuration is necessary.  Change the current directory to !LibPkg
 * then issue the command:
 * @code
 * make all
 * @endcode
 * Please note that some difficulties have been encountered when using
 * recursive makefiles with the experimental 32-bit version of make.
 * They can be overcome by executing the makefiles individually from
 * an obey file.
 * @section support Support
 * The RiscPkg home page may be found at:
 *
 * http://www.riscpkg.org
 *
 * and the library itself downloaded from the same location.
 *
 */
