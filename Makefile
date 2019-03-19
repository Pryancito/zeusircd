all: wellcome
	@if [ ! -f "src/Makefile" ]; then \
		echo "Makefile not found, run configure script before compile the code."; \
		exit; \
		else \
		cd gc; \
		./autogen.sh; \
		./configure --enable-cplusplus --enable-threads=posix --enable-thread-local-alloc --enable-parallel-mark; \
		make -j; \
		make check; \
		make -f Makefile.direct c++; \
		cd ..; \
		cd boost-sources/; \
		./bootstrap.sh --prefix=../boost-compiled --with-libraries=chrono,system,thread,locale; \
		./b2 visibility=global cxxstd=17 install --prefix=../boost-compiled; \
		cd ../src; make; make lang; \
		cd ..; \
	fi


wellcome:
	@echo "****************************************"
	@echo "****************************************"
	@echo "************************ ,--.,---.,----."
	@echo "**** ,---. {code}        |  |  ¬_|  ,--."
	@echo "****    / .---.,   .,---.|  |  __,  |   "
	@echo "****   /  |---'    | ---.|  |  |\  \'--'"
	@echo "**** '---'^---'^---''---'^--^--^ ^--^--'"
	@echo "****************************************"
	@echo "*** { Innovating, Making The World } ***"
	@echo "****************************************"
	@echo "****************************************"
clean:
	@if [ ! -f "src/Makefile" ]; then \
		echo "Makefile not found, run configure script before compile the code."; \
		exit; \
		else \
		cd src; make clean; \
		cd ..; \
	fi
