all: wellcome
	cd src; make; \
	cd ..;
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
	cd src; make clean; \
	cd ..;