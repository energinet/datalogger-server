AC_INIT([RPC Server for Control Daemon], [0.1],
        [LIAB ApS <info@liab.dk>],
        [rpc_server])
#AC_CONFIG_SRCDIR(src/rpserver.c)
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE
#AC_CONFIG_HEADERS(include/config.h)

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL
dnl AC_CONFIG_MACRO_DIR([m4]) dnl CRA 09/02/04
dnl AC_DISABLE_SHARED         dnl CRA 09/02/04

AC_PROG_LIBTOOL           dnl CRA 09/02/05



AC_PATH_PROGS(BASH, bash)

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

dnl Modules...'nuff said.
AC_ARG_WITH(includes,
  [AC_HELP_STRING(
    [--with-includes=LIST],
    [add additional include paths to eesd. LIST is a colon-separated list of include paths to add e.g. --with-includes=/some/mysql/include:/my/include])
  ],
  [ ac_addl_includes=`echo "$withval" | sed -e 's/:/ /g'` ;
    for ainclude in $ac_addl_includes; do
      if test x"$ac_build_addl_includes" = x ; then
        ac_build_addl_includes="-I$ainclude"
      else
        ac_build_addl_includes="-I$ainclude $ac_build_addl_includes"
      fi
    done
    CPPFLAGS="$CPPFLAGS $ac_build_addl_includes"
  ])

CPPFLAGS="$CPPFLAGS -I/usr/include/libxml2"

AC_ARG_WITH(libraries,
  [AC_HELP_STRING(
    [--with-libraries=LIST],
    [add additional library paths eesd LIST is a colon-separated list of include paths to add e.g. --with-libraries=/some/mysql/libdir:/my/libs])
  ],
  [ ac_addl_libdirs=`echo "$withval" | sed -e 's/:/ /g'` ;
    for alibdir in $ac_addl_libdirs; do
      if test x"$ac_build_addl_libdirs" = x ; then
        ac_build_addl_libdirs="-L$alibdir"
      else
        ac_build_addl_libdirs="-L$alibdir $ac_build_addl_libdirs"
      fi
    done 
    LDFLAGS="$LDFLAGS $ac_build_addl_libdirs"
  ])

#AC_CHECK_HEADER(dbi/dbi.h,, AC_MSG_ERROR(dbi/dbi.h not found try using --with-includes ))
#AC_CHECK_LIB(dbi, dbi_initialize,,AC_MSG_ERROR(dbi not found try using --with-libraries))

AC_CHECK_HEADER(pthread.h,, AC_MSG_ERROR(pthread.h not found try using --with-includes ))
AC_CHECK_LIB(pthread, pthread_create,,AC_MSG_ERROR(pthread not found try using --with-libraries))

AC_CHECK_HEADER(dlfcn.h,, AC_MSG_ERROR(dlfcn.h not found try using --with-includes ))
AC_CHECK_LIB(dl, dlsym,,AC_MSG_ERROR(dl not found try using --with-libraries))

#AC_CHECK_HEADER(openssl/md5.h,, AC_MSG_ERROR(openssl/md5.h not found try using --with-includes ))
#AC_CHECK_LIB(ssl, MD5,,AC_MSG_ERROR(openssl not found try using --with-libraries))

#AC_CHECK_HEADER(remboxdb.h,, AC_MSG_ERROR(remboxdb.h not found try using --with-includes ))
#AC_CHECK_LIB(remboxdb, remboxdb_connect,,AC_MSG_ERROR(dl not found try using --with-libraries))

AC_CHECK_HEADER(stdsoap2.h,, AC_MSG_ERROR(stdsoap2.h not found try using --with-includes ))
#AC_CHECK_LIB(gsoap, soap_init,,AC_MSG_ERROR(gsoap not found try using --with-libraries))

AC_PATH_PROG(soapctool, soapcpp2)

PKG_CHECK_MODULES(DEPS_XML, libxml-2.0)
AC_SUBST(DEPS_XML_CFLAGS)
AC_SUBST(DEPS_XML_LIBS)

#AC_CHECK_HEADER(libxmlpatch/xml_patch.h,, AC_MSG_ERROR(xml_patch.h not found try using --with-includes ))
#AC_CHECK_LIB(xmlpatch, xml_patch_add,,AC_MSG_ERROR(xmlpatch not found try using --with-libraries))

dnl Check if svnversion is avaliable to get the rev number
AC_PATH_PROG(SVNVERSION, svnversion)

cgibindir=/var/www/cgi-bin
AC_MSG_CHECKING(cgibindir)
AC_ARG_WITH(cgibindir,
  [AC_HELP_STRING(
    [--with-cgibindir=DIR],
    [Where to put the cgi scripts (default: /var/www/cgi-bin)])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      cgibindir="$withval"
      ;;
    esac])
AC_SUBST(cgibindir)
AC_DEFINE_UNQUOTED(CGIBINDIR,"${cgibindir}",[Directory where module plugin files reside])
AC_MSG_RESULT(${cgibindir})

htmldir=/var/www/htdocs
AC_MSG_CHECKING(htmldir)
AC_ARG_WITH(htmldir,
  [AC_HELP_STRING(
    [--with-htmldir=DIR],
    [Where to put html/js files (default: /var/www/htdocs)])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      htmldir="$withval"
      ;;
    esac])
AC_SUBST(htmldir)
AC_DEFINE_UNQUOTED(HTMLDIR,"${htmldir}",[Directory where html files reside])
AC_MSG_RESULT(${htmldir})

certdir=/etc/certs
AC_MSG_CHECKING(certdir)
AC_ARG_WITH(certdir,
  [AC_HELP_STRING(
    [--with-certdir=DIR],
    [Where to certificats (default: /etc/certs)])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      certdir="$withval"
      ;;
    esac])
AC_SUBST(certdir)
AC_DEFINE_UNQUOTED(CERTDIR,"${certdir}",[Directory where certificats reside])
AC_MSG_RESULT(${certdir})



rpcheader=include/rpserver.h
AC_MSG_CHECKING(rpcheader) 
AC_ARG_WITH(rpcheader,
  [AC_HELP_STRING(
    [--with-rpcheader=DIR],
    [Where to find the rpc header definition])],
  [ case "$withval" in
    yes|no)
    ;;
    * )
      rpcheader="$withval"
      ;;
    esac])
AC_SUBST(rpcheader)
AC_DEFINE_UNQUOTED(RPCHEADER,"${rpcheader}",[Path to the rpcheader])
AC_MSG_RESULT(${rpcheader})



AC_CONFIG_FILES([Makefile gsoap/Makefile src/Makefile])
AC_OUTPUT
