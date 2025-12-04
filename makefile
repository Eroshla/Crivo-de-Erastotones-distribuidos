all: singular distribuido

singular:
	g++ -o singular codigos/singular.cpp -std=c++11 -I.

distribuido:
	mpic++ -o distribuido codigos/distribuido.cpp -std=c++11 -I.

clean:
	rm -f singular distribuido resultados/*.csv resultados/*.txt
