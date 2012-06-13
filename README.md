php_xydiff
==========

## Table of Contents

* [Installation](#installation)
  * [Installing with autotools (Linux / Unix / BSD / Mac OS X)](#unix-installation)
  * [Installing php_xydiff on Windows](#win-installation-top)
      * [Requirements](#win-requirements)
      * [Installation](#win-installation)
      * [Building on windows](#win-build)

Installation
-------------

<a name="unix-installation"></a>
### Installing with autotools (Linux / Unix / BSD / Mac OS X)

 1. Compile xerces-c++ 3.x [from source](http://xerces.apache.org/xerces-c/download.cgi),
    or install binaries + headers for your platform:
  * **Ubuntu / Debian:**
    <pre>apt-get install libxerces-c-dev</pre>
  * **RedHat / CentOS / Fedora**:
    <pre>yum install xerces-c-devel</pre>
  * **Mac OS X ([Homebrew](http://mxcl.github.com/homebrew/))**
    <pre>brew install xerces-c</pre>

 2. Clone the xydiff repository:

        git clone https://github.com/fdintino/xydiff

 3. Change directory to where you cloned the repository and generate the
    files from automake (≥ 1.10), autoconf (≥ 2.61) and m4 (≥ 1.4.6):

        ./autogen.sh

 4. Run a typical configure / make. If xerces-c was installed into an unusual
    location, you may need to pass its prefix to `configure` using, for
    example, `--with-xercesc=/opt/xerces`.

        ./configure
        make
        sudo make install

 4. Check out the source code for php_xydiff:

        git clone https://github.com/fdintino/php_xydiff

 5. Change directory to where you checked out php_xydiff and run the following
    commands. All `--with` arguments are optional; you will only need them if
    configure is unable to determine the install path of libxml, xerces-c, or
    xydiff.
    <pre lang="bash">
	phpize
    ./configure --prefix=$PHP_DIR \
     --with-xydiff=$XYDIFF_SRC_DIR \
     --with-xercesc-dir= $XERCESC_DIR \
     --with-libxml-dir=$LIBXML_DIR
     make
     make test
     sudo make install
     </pre>
 6. Edit your php.ini file, adding the line:
     <pre lang="ini">extension=xydiff.so</pre>

<a name="win-installation-top"></a>
### Installing php_xydiff on Windows

<a name="win-requirements"></a>
#### Requirements

- php 5.3.X compiled for VC9 (Visual C++ 2008)
    * Other extensions compiled for this version of php can be downloaded at (http://downloads.php.net/pierre/).
- Apache 2.X compiled for VC9 ([downloadable at apachehaus.com](http://www.apachehaus.com/cgi-bin/download.plx))

<a name="win-installation"></a>
#### Installation

 1. Compile ([see below](#building-on-windows)) or download [php_xydiff-1.0-php53-vc9-x86.zip](https://github.com/downloads/fdintino/php_xydiff/php_xydiff-1.0-php53-vc9-x86.zip)
 2. Copy **php_xydiff.dll** to `%PHP_DIR%\ext`
 3. Copy **xerces-c_3_0.dll** into the `%PHP_DIR%`. (Should be in zip binary, or in `C:\code\xerces-c-3.0.0-x86-windows-vc-9.0\bin` if you're [compiling yourself](#building-on-windows))
 4. Edit your php.ini file, adding the line:
    <pre lang="ini">extension=php_xydiff.dll</pre>

<a name="win-build"></a>
#### Building on windows

 1. Make sure Microsoft Visual Studio 2008 express is installed
    ([download Visual C++ 2008 express installer](http://download.microsoft.com/download/A/5/4/A54BADB6-9C3F-478D-8657-93B3FC9FE62D/vcsetup.exe)).
 2. Install the Microsoft Windows SDK v6.1 if it is not already installed
    ([download](http://www.microsoft.com/downloads/details.aspx?FamilyID=e6e1c3df-a74f-4207-8586-711ebe331cdc&DisplayLang=en)).
    Restart after installing.
 3. Create a working directory for the code. We will assume from here on in
    that this directory is `C:\code`.
 4. Download the [Xerces 3.0.0 binaries compiled for Windows VC9](http://archive.apache.org/dist/xerces/c/3/binaries/xerces-c-3.0.0-x86-windows-vc-9.0.zip)
    and extract them in `C:\code`. They should now be in the folder
    `C:\code\xerces-c-3.0.0-x86-windows-vc-9.0`.
 5. Check out xydiff into `C:\code\xydiff`:
    <pre lang="bash">git clone https://github.com/fdintino/xydiff</pre>
 6. Open `C:\code\xydiff\vc9.0\xydiff.sln`. Choose "Release" as the active
    configuration, and build the solution
 7. <a name="footnote-back-1"></a>Unzip the
    [php_build_53_x86_vc9.zip](https://github.com/downloads/fdintino/php_xydiff/php_build_53_x86_vc9.zip)
    file in the **Downloads** section of this repo into `C:\code`. The
    contents should now be in the directory `C:\code\php_build_53_x86_vc9`.
    <sup>**[1](#footnote-1)**</sup>
 8. Create the directory `C:\usr\local\share` and copy
    `C:\code\php_build_53_x86_vc9\bin\bison.simple` to this directory.
 8. Get the source code for php 5.3.2
    ([download](http://us.php.net/get/php-5.3.2.tar.gz/from/this/mirror))
    and extract it into `C:\code`. The contents should now be in the directory
    `C:\code\php-5.3.2`.
 9. Check out the source code for php_xydiff into `C:\code\php-5.3.2\ext\xydiff`.
    <pre lang="bash">git clone https://github.com/fdintino/php_xydiff C:\code\php-5.3.2\ext\xydiff</pre>
 10. Open Start → Programs → Microsoft Windows SDK v6.1 → CMD Shell
 11. Issue the following commands:

```bat
"C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
[if on 64 bit windows: "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"]
setenv /x86 /xp /release
set PATH=%PATH%;"C:\code\php_build_53_x86_vc9\bin"
cd C:\code\php-5.3.2
buildconf.bat
cscript /nologo configure.js --with-xydiff ^
       --with-php-build="C:\code\php_build_53_x86_vc9" ^
       --with-extra-includes="C:\code\xydiff;C:\code\xydiff\include;C:\code\xerces-c-3.0.0-x86-windows-vc-9.0\include" ^
       --with-extra-libs="C:\code\xydiff\vc9.0\Release;C:\code\xerces-c-3.0.0-x86-windows-vc-9.0\lib"
nmake clean
nmake
```

### Footnotes

1. <a name="footnote-1"></a>[php_build_53_x86_vc9.zip](https://github.com/downloads/fdintino/php_xydiff/php_build_53_x86_vc9.zip)
is a combined zip of the following files/libraries upon which php 5.3.X depends:
  *  http://files.edin.dk/php/win32/zip.zip
  *  http://pecl2.php.net/downloads/php-windows-builds/php-libs/binary-tools.zip
  *  http://pecl2.php.net/downloads/php-windows-builds/php-libs/VC9/x86/libxml2-2.7.3-vc9-x86.zip
  *  http://pecl2.php.net/downloads/php-windows-builds/php-libs/VC9/x86/zlib-1.2.3-vc9-x86.zip
  *  http://pecl2.php.net/downloads/php-windows-builds/php-libs/VC9/x86/libiconv-1.12-vc9-x86.zip
  *  http://pecl2.php.net/downloads/php-windows-builds/php-libs/VC9/x86/ICU-3.8.1-vc9-x86.zip
For reference see [PHP Wiki internals:windows:libs](http://wiki.php.net/internals/windows/libs)

  **[[back]](#footnote-back-1)**