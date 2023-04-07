#!/usr/bin/env bash

echo -e "Beginning installation"
debian()
{
	sudo apt-get -y update
	sudo apt-get -y install gcc g++ cmake cmake-curses-gui uuid-dev libssl-dev libsasl2-2 libsasl2-dev libsasl2-modules swig python-dev ruby-dev wget qpid-proton
	sudo apt-get -y install nano gcc cpp g++ libssl-dev libsqlite3-dev git make gettext libicu-dev openssl libmaxminddb0 libmaxminddb-dev mmdb-bin libmariadb-dev libyaml-cpp-dev libmariadb-dev-compat
	./configure
	make
}

ubuntu()
{
	sudo apt-get -y update
	sudo apt-get -y install gcc g++ cmake cmake-curses-gui uuid-dev libssl-dev libsasl2-2 libsasl2-dev libsasl2-modules swig python-dev ruby-dev wget qpid-proton
	sudo apt-get -y install nano gcc cpp g++ libssl-dev libsqlite3-dev git make gettext libicu-dev openssl libmaxminddb0 libmaxminddb-dev mmdb-bin libmariadb-dev libyaml-cpp-dev libmariadb-dev-compat
	./configure
	make
}

redhat()
{
	sudo dnf group install -y "Development Tools"
	sudo dnf install -y python3 cyrus-sasl ruby-devel uuid-devel swig wget cmake
	sudo yum install wget nano gcc cpp gcc-c++ openssl-devel sqlite-devel git make libicu-devel gettext libmaxminddb-devel mariadb-client mariadb-devel yaml-cpp-devel qpid-proton
	./configure
	make
}

lnx()
{
	DISTRO=$(cat /etc/os-release | grep ^ID_LIKE | tr -d 'ID_LIKE="')
	case "$DISTRO" in
		debian*)  debian;; 
		ubuntu*)   ubuntu;;
		rhel*)     redhat;;
		*)        echo -e "unknown linux distribution: $DISTRO";;
	esac
}

bsd()
{
	sudo pkg update
        sudo pkg install python3 cyrus-sasl ruby uuid swig wget cmake sudo qpid-proton
        sudo pkg install gcc9 openssl sqlite3 gmake gettext git bash nano libmaxminddb mariadb105-client cmake yaml-cpp
        ./configure
        gmake
}

mac()
{
	brew update
	for program in python cyrus-sasl ruby uuid swig wget cmake sudo qpid-proton; do brew install $program; done
	for program in nano gcc9 openssl sqlite git gmake gettext libmaxminddb mysql yaml-cpp; do brew install $program; done
	sudo ln -s /usr/local/opt/openssl/include/openssl /usr/local/include
	./configure
	make
}

cygwin()
{
	setup-x86_64.exe -q -s http://cygwin.mirror.constant.com -P "gcc g++ cmake cmake-curses-gui uuid-devel libssl-devel libsasl2-2 libsasl2-devel libsasl2-modules swig python3-devel ruby-devel wget"
	qpid
	setup-x86_64.exe -q -s http://cygwin.mirror.constant.com -P "gcc9 sqlite3 make gmake gettext git bash nano libmaxminddb mariadb105-client cmake yaml-cpp"
	./configure
	gmake
}

case "$OSTYPE" in
        linux*)   lnx;;
        bsd*)     bsd;;
        cygwin*)  cygwin;;
	darwin*)  mac;;
        *)        echo -e "unknown operating system: $OSTYPE";;
esac
