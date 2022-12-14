#!/usr/bin/env  bash

qpid()
{
	$(wget https://www.zeusircd.net/qpid-proton-0.30.0.tar.gz)
        $(tar -xzf qpid-proton-0.30.0.tar.gz)
        $(cd qpid-proton-0.30.0)
        $(mkdir build)
        $(cd build)
        $(cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DSYSINSTALL_BINDINGS=ON)
        $(make)
        $(sudo make install)
        $(cd ../..)
}

macos()
{
	$(brew update)
	$(brew install python cyrus-sasl ruby uuid swig wget cmake sudo)
	qpid
	$(brew install nano gcc cpp gcc-c++ openssl sqlite git gmake libicu gettext libmaxminddb mysql-client yaml-cpp)
	$(./configure)
	$(gmake)
}

linux()
{
	DISTRO=$(cat /etc/*release | grep ^ID_LIKE | tr -d 'ID_LIKE="')
	if ($DISTRO == "debian") then
		$(sudo apt update)
		$(sudo apt install gcc g++ cmake cmake-curses-gui uuid-dev libssl-dev libsasl2-2 libsasl2-dev libsasl2-modules swig python-dev ruby-dev wget)
		qpid
		$(apt install nano gcc cpp g++ libssl-dev libsqlite3-dev git make gettext libicu-dev openssl libmaxminddb0 libmaxminddb-dev mmdb-bin libmariadb-dev libyaml-cpp-dev libmariadb-dev-compat)
		$(./configure)
		$(make)
	elif ($DISTRO == "ubuntu") then
		$(sudo apt update)
                $(sudo apt install gcc g++ cmake cmake-curses-gui uuid-dev libssl-dev libsasl2-2 libsasl2-dev libsasl2-modules swig python-dev ruby-dev wget)
                qpid
                $(apt install nano gcc cpp g++ libssl-dev libsqlite3-dev git make gettext libicu-dev openssl libmaxminddb0 libmaxminddb-dev mmdb-bin libmariadb-dev libyaml-cpp-dev libmariadb-dev-compat)
                $(./configure)
                $(make)
	elif ($DISTRO == "rhel fedora") then
		$(sudo dnf group install -y "Development Tools")
		$(sudo dnf install -y python3 cyrus-sasl ruby-devel uuid-devel swig wget cmake)
                qpid
                $(yum install wget nano gcc cpp gcc-c++ openssl-devel sqlite-devel git make libicu-devel gettext libmaxminddb-devel mariadb-client mariadb-devel yaml-cpp-devel)
                $(./configure)
                $(make)
	else
		echo -e "Unsuported Linux Operating system, can you install manually ?"
	fi
}

bsd()
{
	$(pkg update)
        $(pkg install python3 cyrus-sasl ruby uuid swig wget cmake sudo)
        qpid
        $(pkg install gcc9 sqlite3 gmake gettext git bash nano libmaxminddb mariadb105-client cmake yaml-cpp)
        $(./configure)
        $(gmake}
}

cygwin()
{
	$(setup-x86_64.exe -q -s http://cygwin.mirror.constant.com -P "gcc g++ cmake cmake-curses-gui uuid-devel libssl-devel libsasl2-2 libsasl2-devel libsasl2-modules swig python3-devel ruby-devel wget")
	qpid
	$(setup-x86_64.exe -q -s http://cygwin.mirror.constant.com -P "gcc9 sqlite3 make gmake gettext git bash nano libmaxminddb mariadb105-client cmake yaml-cpp")
	$(./configure)
	$(gmake)
}

case "$OSTYPE" in
  darwin*)  macos ;; 
  linux*)   linux ;;
  bsd*)     bsd ;;
  cygwin*)  cygwin ;;
  *)        echo -e "unknown operating system: $OSTYPE" ;;
esac
