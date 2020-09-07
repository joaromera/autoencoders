INC=-I../../AutoencoderJuce/external/frugally-deep/include -I../../AutoencoderJuce/external/eigen -I../../AutoencoderJuce/external/FunctionalPlus/include -I../../AutoencoderJuce/external/json/include

default:	main

main:
	g++ -Wall -std=c++17 $(INC) integratingFrugallyDeepTests.cpp -o integratingFrugallyDeepTests
	g++ -Wall -std=c++17 $(INC) interp1dTests.cpp -o interp1dTests

clean:
	rm -r interp1dTests integratingFrugallyDeepTests