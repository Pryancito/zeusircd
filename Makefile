all: wellcome
	@if [ ! -f "src/Makefile" ]; then \
		echo "Makefile not found, run configure script before compile the code."; \
		exit; \
		else \
		cd src; make; make lang; \
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
