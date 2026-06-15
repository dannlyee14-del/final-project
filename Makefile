all:
	clear
	g++ main.cpp library.cpp book.cpp page.cpp -I . -std=c++11 -o Final03
	./Final03

run: 
	./Final03

clean:
	rm -f Final03
